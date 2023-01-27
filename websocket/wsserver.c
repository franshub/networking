/* wsserver.c */

/* This is a very rudimentary websocket server.  It accepts an HTTP
 * connection is expected to upgrade to websocket. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "sha1.h"
#include "base64.h"

static void release_lines(char **lines) {
    if (lines == NULL) {
        return;
    }
    for (char **p = lines; *p != NULL; p++) {
        free(*p);
    }
    free(lines);
}

/* Returns a list of lines.  Or NULL. */
static char **read_request(int sock) {
    printf("waiting for request...\n");
    int line_count = 0;
    char **lines = NULL;
    char buffer[1024];
    int offset = 0;
    while (1) {
        ssize_t nr_recv = recv(sock, buffer + offset, sizeof(buffer) - offset, 0);
        if (nr_recv <= 0) {
            printf(nr_recv == 0 ? "connection closed unexpectedly\n" : "connection lost\n");
            release_lines(lines);
            return NULL;
        }

        int nr_in_buf = offset + nr_recv;
        char *start_of_line = buffer;
        for (char *p = buffer; p < buffer + nr_in_buf - 1; p++) {
            if (*p == 0x0d &&  *(p + 1) == 0x0a) {
                *p = '\0';
                if (lines == NULL) {
                    line_count = 1;
                    lines = (char **) malloc((line_count + 1) * sizeof(char **));
                    if (lines == NULL) {
                        printf("out of memory (1)\n");
                        return NULL;
                    }
                } else {
                    line_count++;
                    char **new_lines = (char **) realloc(lines, (line_count + 1) * sizeof(char **));
                    if (new_lines == NULL) {
                        printf("out of memory (2)\n");
                        release_lines(lines);
                        return NULL;
                    }
                    lines = new_lines;
                }
                lines[line_count] = NULL;
                lines[line_count - 1] = strdup(start_of_line);
                if (lines[line_count - 1] == NULL) {
                    printf("out of memory (3)\n");
                    release_lines(lines);
                    return NULL;
                }
                if (strlen(start_of_line) == 0) {
                    return lines;
                }
                start_of_line = p + 2;
            }
        }

        int remaining = nr_in_buf - (start_of_line - buffer);
        if (remaining > 0) {
            memmove(buffer, start_of_line, remaining);
        }
        offset = remaining;
    }
}

static int send_line(int sock, char *line) {
    printf("%s", line);
    ssize_t nr_sent = send(sock, line, strlen(line), 0);
    if (nr_sent != strlen(line)) {
        printf("failed to send all bytes\n");
        return 0;
    }
}

static char *header_value(char *line) {
    for (; *line != ':'; line++) {
        if (*line == '\0') {
            return NULL;
        }
    }
    line++;
    while (isspace(*line)) {
        line++;
    }
    return line;
}

static int startswith(char *line, char *keyword) {
    return strncasecmp(line, keyword, strlen(keyword)) == 0;
}

static int handle_client_handshake(int sock) {
    char **request = read_request(sock);
    if (request == NULL) {
        return 0;
    }
    printf("received request:\n-----\n");
    for (char **p = request; *p != NULL; p++) {
        printf("%s\n", *p);
    }
    printf("-----\n");

    char *sec_websocket_key = NULL;
    for (char **p = request; *p != NULL; p++) {
        if (startswith(*p, "Sec-WebSocket-Key")) {
            sec_websocket_key = header_value(*p);
            break;
        }
    }
    if (sec_websocket_key == NULL) {
        printf("Sending response:\n");
        send_line(sock, "HTTP/1.1 400 Bad Request\r\n\r\n");
        printf("-----\n");
        release_lines(request);
        return 0;
    }

    char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char keybuf[strlen(sec_websocket_key) + strlen(guid) + 1];
    strcpy(keybuf, sec_websocket_key);
    strcat(keybuf, guid);
    unsigned char *hashed = sha1(keybuf, strlen(keybuf));
    char *encoded = base64_encode(hashed, 20);
    free(hashed);
    release_lines(request);

    printf("Sending response:\n");
    send_line(sock, "HTTP/1.1 101 Switching Protocols\r\n");
    send_line(sock, "Upgrade: websocket\r\n");
    send_line(sock, "Connection: Upgrade\r\n");
    send_line(sock, "Sec-WebSocket-Accept: ");
    send_line(sock, encoded);
    send_line(sock, "\r\n");
    send_line(sock, "\r\n");
    printf("-----\n");

    free(encoded);

    return 1;
}

struct frame_header {
    int fin;
    int rsv1;
    int rsv2;
    int rsv3;
    int opcode;
    int has_mask;
    unsigned long length_msw;
    unsigned long length_lsw;
    unsigned char masking_key[4];
};

/*
 * Raw header (bit 0 is most significant):
 * offset  size  content
 *    0.0   0.1  fin bit (indicates last frame of message)
 *    0.1   0.1  rsv1 bit (must be 0)
 *    0.2   0.1  rsv2 bit (must be 0)
 *    0.3   0.1  rsv3 bit (must be 0)
 *    0.4   0.4  opcode
 *    1.0   0.1  mask bit
 *    1.1   0.7  payload length
 *    2     2    actual payload length if payload length is 126 or 127
 *    4     6    actual payload length continued if payload length is 127
 *   10     4    masking key (if mask bit is 1)
 */

#define OPCODE_CONTINUATION  0x00
#define OPCODE_TEXT          0x01
#define OPCODE_BINARY        0x02
#define OPCODE_CLOSE         0x08
#define OPCODE_PING          0x09
#define OPCODE_PONG          0x0a

static void send_text_frame(int sock, char *message) {
    int header_length = 2;
    unsigned char header[10];
    header[0] = 0x80 | OPCODE_TEXT;

    unsigned long body_length = strlen(message);
    if (body_length > 0xffff) {
        header[1] = 127;
        for (int i = 0; i < 8; i++) {
            header[2 + i] = (body_length >> (8 * i)) & 0xff;
        }
        header_length += 8;
    } else if (body_length > 126) {
        header[1] = 126;
        header[2] = (body_length >> 8) & 0xff;
        header[3] = body_length & 0xff;
        header_length += 2;
    } else {
        header[1] = body_length;
    }
    send(sock, header, header_length, 0);
    send(sock, message, strlen(message), 0);
}

static void send_close_frame(int sock) {
    unsigned char buf[2] = { 0x88, 0x00 };
    send(sock, buf, sizeof(buf), 0);
}

static int read_frame_body(int sock, struct frame_header *header) {
    printf("body: ");

    unsigned long msw = header->length_msw;
    unsigned long lsw = header->length_lsw;
    if (msw == 0 && lsw == 0) {
        printf("<empty>\n");
        return 1;
    }

    printf("\"");
    int mask_ix = 0;
    while (msw > 0 || lsw > 0) {
        unsigned char buffer[256];
        int chunksize = (lsw > 0 && lsw < sizeof(buffer)) ? lsw : sizeof(buffer);
        ssize_t nr_recv = recv(sock, buffer, chunksize, 0);
        if (nr_recv <= 0) {
            printf(nr_recv == 0 ? "\nconnection closed unexpectedly\n" : "\nconnection lost\n");
            return 0;
        }
        if (lsw > 0) {
            lsw -= nr_recv;
        } else {
            lsw = (unsigned long) 0xffffffff - (nr_recv - 1);
            msw--;
        }

        for (int i = 0; i < nr_recv; i++) {
            unsigned char ch = buffer[i] ^ header->masking_key[mask_ix];
            mask_ix = (mask_ix + 1) % 4;
            if (isprint(ch)) {
                printf("%c", ch);
            } else {
                printf("\\x%02.2x", ch);
            }
        }
    }
    printf("\"\n");

    return 1;
}

static int parse_frame_header(unsigned char *buffer, struct frame_header *header) {
    header->fin = (buffer[0] & 0x80) >> 7;
    header->rsv1 = (buffer[0] & 0x40) >> 6;
    header->rsv2 = (buffer[0] & 0x20) >> 5;
    header->rsv3 = (buffer[0] & 0x10) >> 4;
    header->opcode = buffer[0] & 0x0f;
    header->has_mask = (buffer[1] & 0x80) >> 7;

    unsigned int payload_length = buffer[1] & 0x7f;
    int mask_offset = 2;
    if (payload_length < 126) {
        header->length_lsw = payload_length;
        header->length_msw = 0;
    } else if (payload_length == 126) {
        header->length_lsw = (buffer[2] << 8) | buffer[9];
        header->length_msw = 0;
        mask_offset += 2;
    } else {
        header->length_lsw = (buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
        header->length_msw = (buffer[6] << 24) | (buffer[7] << 16) | (buffer[8] << 8) | buffer[9];
        mask_offset += 4;
    }

    if (header->has_mask) {
        for (int i = 0; i < 4; i++) {
            header->masking_key[i] = buffer[mask_offset + i];
        }
    } else {
        memset(header->masking_key, 0, 4);
    }

    static char *opcode_names[] = {
        "continuation", "text", "binary",
        "reserved", "reserved", "reserved", "reserved", "reserved",
        "close", "ping", "pong",
        "reserved", "reserved", "reserved", "reserved", "reserved"
    };
    printf("frame:");
    if (header->rsv1 != 0) {
        printf(" rsv1=1");
    }
    if (header->rsv2 != 0) {
        printf(" rsv2=1");
    }
    if (header->rsv3 != 0) {
        printf(" rsv3=1");
    }
    printf(" %d %s", header->opcode, opcode_names[header->opcode]);
    if (!header->fin) {
        printf(" not final");
    }
    if (header->length_msw > 0) {
        printf(" %lu * 2^32 + ", header->length_msw);
    }
    printf(" %lu bytes", header->length_lsw);
    if (header->has_mask) {
        printf(" masked");
    }
    printf(" ");

    return 1;
}

static int read_frame_header(int sock, struct frame_header *header) {
    unsigned char buffer[14];
    int byte_count = 0;
    int needed_byte_count = 2;
    while (byte_count < needed_byte_count) {
        ssize_t nr_recv = recv(sock, buffer + byte_count, needed_byte_count - byte_count, 0);
        if (nr_recv <= 0) {
            printf(nr_recv == 0 ? "connection closed unexpectedly\n" : "connection lost\n");
            return 0;
        }
        byte_count += nr_recv;
        if (byte_count >= 2 && needed_byte_count == 2) {
            if (buffer[1] & 0x80) {
                // mask bit is set
                needed_byte_count += 4;
            }
            int payload_length = buffer[1] & 0x7f;
            if (payload_length == 126) {
                needed_byte_count += 2;
            } else if (payload_length == 127) {
                needed_byte_count += 8;
            }
        }
    }

    return parse_frame_header(buffer, header);
}

static void handle_client(int sock) {
    int ok = handle_client_handshake(sock);
    if (!ok) {
        close(sock);
        return;
    }

    while (1) {
        struct frame_header header;
        int ok = read_frame_header(sock, &header);
        if (!ok) {
            break;
        }
        ok = read_frame_body(sock, &header);
        if (!ok) {
            break;
        }
        if (header.opcode == OPCODE_CLOSE) {
            send_close_frame(sock);
            break;
        }
        send_text_frame(sock, "you said something");
    }

    close(sock);
}

static char *sa_tostring(struct sockaddr *sa, char *buf) {
    if (sa->sa_family != AF_INET) {
        sprintf(buf, "unknown address family %d", sa->sa_family);
        return buf;
    }

    /* note that inet_ntoa() is not thread safe */
    struct sockaddr_in *sin = (struct sockaddr_in *) sa;
    sprintf(buf, "%s:%d",
            inet_ntoa(sin->sin_addr),
            ntohs(sin->sin_port));
    return buf;
}

static void handle_incoming(int sock) {
    struct sockaddr_in remote_addr;
    socklen_t remote_size = sizeof(remote_addr);

    printf("accepting incoming http-websocket connections...\n");
    int csock = accept(sock, (struct sockaddr *) &remote_addr, &remote_size);
    if (csock < 0) {
        printf("accept() failed\n");
        return;
    }

    char addr_s[64];
    sa_tostring((struct sockaddr *) &remote_addr, addr_s);
    printf("connected with %s\n", addr_s);

    handle_client(csock);
}

/**
 * Creates a socket and binds it to the specified port.
 * Returns the socket, or <0 in case of trouble.
 */
static int create_socket(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("socket() failed\n");
        return -1;
    }
    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) != 0) {
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen_t addr_len = sizeof(addr);
    if (bind(sock, (struct sockaddr *) &addr, addr_len) != 0) {
        printf("bind() to port %d in INADDR_ANY failed\n", port);
        close(sock);
        return -1;
    }

    if (listen(sock, 0) != 0) {
        printf("listen() failed\n");
        close(sock);
        return -1;
    }

    printf("socket bound to port %d\n", port);
    return sock;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("expected a port number\n");
        return 0;
    }

    uint16_t port = (uint16_t) atoi(argv[1]);
    int sock = create_socket(port);

    handle_incoming(sock);

    close(sock);

    return 0;
}

/*
 * Client request, per RFC6455:
 *
 * 7- The request MUST include a header field with the name
 * |Sec-WebSocket-Key|.  The value of this header field MUST be a
 * nonce consisting of a randomly selected 16-byte value that has been
 * base64-encoded (see Section 4 of [RFC4648]).  The nonce MUST be
 * selected randomly for each connection.
 *
 * Server response:
 *
 * HTTP/1.1 101 Switching Protocols
 * Upgrade: websocket
 * Connection: Upgrade
 * Sec-WebSocket-Accept: <see below>
 *
 * The Sec-WebSocket-Accept value must be the base64-encoded SHA-1 of
 * the concatenation of the Sec-WebSocket-Key (as a string, not
 * base64-decoded) with the string
 * "258EAFA5-E914-47DA-95CA-C5AB0DC85B11".
 */
