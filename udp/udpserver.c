/* udpserver.c */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

char *sa_tostring(struct sockaddr *sa, char *buf) {
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

/**
 * Creates a socket and binds it to the specified port.
 * Returns the socket, or <0 in case of trouble.
 */
int create_socket(uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("socket() failed\n");
        return -1;
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

    printf("socket bound to port %d\n", port);
    return sock;
}

#define MAX_MESSAGE_SIZE  512

void handle_incoming(int sock) {
    printf("waiting for incoming udp message...\n");
    char buf[MAX_MESSAGE_SIZE];
    struct sockaddr_in remote_addr;
    socklen_t remote_size = (socklen_t) sizeof(remote_addr);
    int nr_recv = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr *) &remote_addr,
                           &remote_size);
    if (nr_recv < 0) {
        printf("recvfrom() failed\n");
        return;
    }
    char addr_s[64];
    sa_tostring((struct sockaddr *) &remote_addr, addr_s);
    printf("received %d bytes from %s: '", nr_recv, addr_s);
    int i;
    for (i = 0; i < nr_recv; i++) {
        if (isprint(buf[i])) {
            putchar(buf[i]);
        } else {
            printf("\\x%02.2x", buf[i]);
        }
    }
    putchar('\'');
    putchar('\n');

    printf("sending reply...\n");
    char reply[256];
    sprintf(reply, "Greetings back, %s.\n", addr_s);
    ssize_t nr_sent = sendto(sock, reply, strlen(reply), 0,
                             (struct sockaddr *) &remote_addr,
                             remote_size);
    if (nr_sent != strlen(reply)) {
        printf("failed to send all bytes: %ld\n", (long) nr_sent);
    }

    printf("ready\n");
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
