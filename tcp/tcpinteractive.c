/* tcpinteractive.c */

/* interactive TDP client */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>

struct control_s {
    int sock;
    int stop_recv;           /* this is a boolean */
    pthread_mutex_t mutex;
    struct sockaddr_in addr;
};

int prepare_for_client(struct control_s *cs,
                       char *ipaddress,
                       uint16_t port) {
    cs->sock = socket(AF_INET, SOCK_STREAM, 0);
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

    return 1;
}

void *do_receive(void *arg) {
    struct control_s *cs = (struct control_s *) arg;

    pthread_mutex_lock(&cs->mutex);
    printf("receiving from socket %d\n", cs->sock);
    pthread_mutex_unlock(&cs->mutex);

    if (connect(cs->sock, (struct sockaddr *) &(cs->addr), sizeof(cs->addr)) != 0) {
        pthread_mutex_lock(&cs->mutex);
        printf("connect() failed\n");
        pthread_mutex_unlock(&cs->mutex);
        cs->stop_recv = 1;
        return NULL;
    }

    while (!cs->stop_recv) {
        pthread_mutex_lock(&cs->mutex);
        printf("waiting for data...\n");
        pthread_mutex_unlock(&cs->mutex);

        char data[256];
        ssize_t nr_recv = recv(cs->sock, data, sizeof(data), 0);
        if (nr_recv < 0) {
            pthread_mutex_lock(&cs->mutex);
            printf("recv() failed\n");
            pthread_mutex_unlock(&cs->mutex);
            cs->stop_recv = 1;
            return NULL;
        }
        if (nr_recv == 0) {
            pthread_mutex_lock(&cs->mutex);
            printf("connection was closed by server\n");
            pthread_mutex_unlock(&cs->mutex);
            cs->stop_recv = 1;
            return NULL;
        }

        pthread_mutex_lock(&cs->mutex);
        printf("received %ld bytes:", (long) nr_recv);
        int i;
        for (i = 0; i < nr_recv; i++) {
            printf(" %02.2x", data[i]);
        }
        putchar('\n');
        pthread_mutex_unlock(&cs->mutex);
    }

    return NULL;
}

void do_send(struct control_s *cs) {
    pthread_mutex_lock(&cs->mutex);
    printf("Type bytes as one or two character hexadecimal values.\n");
    printf("Separate bytes by a space.\n");
    printf("Press <enter> to send.\n");
    printf("Type stop<enter> to quit.\n");
    pthread_mutex_unlock(&cs->mutex);

    char buf[256];
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (cs->stop_recv) {
            break;
        }
        if (memcmp(buf, "stop", 4) == 0) {
            break;
        }
        int nr_bytes = 0;
        char *cp;
        unsigned char value = 0;
        for (cp = buf; *cp != '\0'; cp++) {
            if (isdigit(*cp)) {
                value <<= 4;
                value |= *cp - '0';
                continue;
            }
            if (isalpha(*cp)) {
                value <<= 4;
                value |= tolower(*cp) - 'a' + 10;
                continue;
            }
            if (!isspace(*cp)) {
                continue;
            }
            if (cp > buf) {
                buf[nr_bytes] = ((int) value) & 0xff;
                value = 0;
                nr_bytes++;
            }
            if (*cp != '\n') {
                continue;
            }
            if (nr_bytes <= 0) {
                break;
            }
            ssize_t nr_sent = send(cs->sock, buf, nr_bytes, 0);
            pthread_mutex_lock(&cs->mutex);
            printf("sent %ld %s\n", (long) nr_sent, (nr_sent == 1 ? "byte" : "bytes"));
            pthread_mutex_unlock(&cs->mutex);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    char *ipaddress;
    uint16_t port;
    if (argc < 3) {
        printf("expected an ip address and a port number\n");
        return 0;
    }
    ipaddress = argv[1];
    port = (uint16_t) atoi(argv[2]);

    struct control_s cs;
    int ok = prepare_for_client(&cs, ipaddress, port);
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
