/* udpclienti.c */

/* interactive UDP client */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct control_s {
    int sock;
    int stop_recv;           /* this is a boolean */
    pthread_mutex_t mutex;
    int has_addr;
    struct sockaddr_in addr;
};

int prepare_for_client(struct control_s *cs,
                       char *ipaddress,
                       uint16_t port) {
    cs->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (cs->sock < 0) {
        printf("failed to create socket\n");
        return 0;
    }

    in_addr_t in_addr = inet_addr(ipaddress);
    if (in_addr == (in_addr_t) -1) {
        printf("failed to parse ip address '%s'\n", ipaddress);
        return 0;
    }

    memset(&cs->addr, 0, sizeof(cs->addr));
    cs->addr.sin_family = AF_INET;
    cs->addr.sin_port = htons(port);
    cs->addr.sin_addr.s_addr = in_addr;

    cs->has_addr = 1;

    return 1;
}

int prepare_for_server(struct control_s *cs,
                       uint16_t port) {
    cs->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (cs->sock < 0) {
        printf("failed to create socket\n");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    socklen_t addr_len = sizeof(addr);
    if (bind(cs->sock, (struct sockaddr *) &addr, addr_len) != 0) {
        printf("failed to bind socket to port %d\n", port);
        close(cs->sock);
        return -1;
    }

    cs->has_addr = 0;

    return 1;
}

void *do_receive(void *arg) {
    struct control_s *cs = (struct control_s *) arg;

    pthread_mutex_lock(&cs->mutex);
    printf("receiving from socket %d\n", cs->sock);
    pthread_mutex_unlock(&cs->mutex);

    char buf[256];
    struct sockaddr_in remote_addr;

    while (!cs->stop_recv) {
        int remote_size = sizeof(remote_addr);
        int nr_recv = recvfrom(cs->sock, buf, sizeof(buf), 0,
                               (struct sockaddr *) &remote_addr,
                               &remote_size);
        if (nr_recv < 0) {
            if (!cs->stop_recv) {
                pthread_mutex_lock(&cs->mutex);
                printf("recvfrom() failed\n");
                pthread_mutex_unlock(&cs->mutex);
            }
            return;
        }

        pthread_mutex_lock(&cs->mutex);
        memcpy(&cs->addr, &remote_addr, sizeof(remote_addr));
        cs->has_addr = 1;
        printf("received %d %s: '", nr_recv, (nr_recv == 1 ? "byte" : "bytes"));
        int i;
        for (i = 0; i < nr_recv; i++) {
            if (i > 0) {
                putchar(' ');
            }
            printf("%02.2x", buf[i]);
        }
        putchar('\'');
        putchar('\n');
        pthread_mutex_unlock(&cs->mutex);
    }
}

void do_send(struct control_s *cs) {
    pthread_mutex_lock(&cs->mutex);
    printf("Type bytes as two character hexadecimal values.\n");
    printf("Separate bytes by a space.\n");
    printf("Press <enter> to send.\n");
    printf("Type stop<enter> to quit.\n");
    pthread_mutex_unlock(&cs->mutex);

    char buf[256];
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (memcmp(buf, "stop", 4) == 0) {
            break;
        }
        int nr_bytes = 0;
        char *cp;
        char value = 0;
        for (cp = buf; *cp != '\0'; cp++) {
            if (isdigit(*cp)) {
                value <<= 4;
                value |= *cp - '0';
                continue;
            }
            if (isalpha(*cp)) {
                value <<= 4;
                value |= tolower(*cp) - 'a';
                continue;
            }
            if (!isspace(*cp)) {
                continue;
            }
            if (cp > buf) {
                buf[nr_bytes] = value;
                value = 0;
                nr_bytes++;
            }
            if (*cp != '\n') {
                continue;
            }
            if (nr_bytes <= 0) {
                break;
            }
            if (!cs->has_addr) {
                pthread_mutex_lock(&cs->mutex);
                printf("Cannot send: peer address is not yet known.\n");
                pthread_mutex_unlock(&cs->mutex);
            } else {
                ssize_t nr_sent = sendto(cs->sock, buf, nr_bytes, 0,
                                         (struct sockaddr *) &cs->addr,
                                         sizeof(cs->addr));
                pthread_mutex_lock(&cs->mutex);
                printf("sent %d %s", nr_sent, (nr_sent == 1 ? "byte" : "bytes"));
                pthread_mutex_unlock(&cs->mutex);
            }
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    struct control_s cs;
    int ok = 1;

    int is_server = 0;
    char *ipaddress;
    uint16_t port;
    if (argc >= 3) {
        ipaddress = argv[1];
        port = (uint16_t) atoi(argv[2]);
        ok = prepare_for_client(&cs, ipaddress, port);
    } else if (argc >= 2) {
        port = (uint16_t) atoi(argv[1]);
        ok = prepare_for_server(&cs, port);
    } else {
        printf("expected a port number (for server operation),\n");
        printf("or an ip address and a port number (for client operation)\n");
        return 0;
    }
    if (!ok) {
        return -1;
    }

    pthread_mutex_init(&cs.mutex, NULL);

    pthread_t recv_thread;
    cs.stop_recv = 0;
    pthread_create(&recv_thread, 0, do_receive, (void *) &cs);

    do_send(&cs);

    cs.stop_recv = 1;
    close(cs.sock);
    pthread_join(recv_thread, 0);
    pthread_mutex_destroy(&cs.mutex);

    return 0;
}
