/* udpclient.c */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("expected an ip address and a port number\n");
        return 0;
    }

    char *ipaddress = argv[1];
    uint16_t port = (uint16_t) atoi(argv[2]);

    in_addr_t in_addr = inet_addr(ipaddress);
    if (in_addr == (in_addr_t) -1) {
        printf("failed to parse ip address '%s'\n", ipaddress);
        return 0;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("socket() failed\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = in_addr;

    char addr_s[64];
    sa_tostring((struct sockaddr *) &server_addr, addr_s);
    printf("sending greeting to %s...\n", addr_s);

    char buf[256];
    sprintf(buf, "Hello, server!\n");
    ssize_t nr_sent = sendto(sock, buf, strlen(buf), 0,
                             (struct sockaddr *) &server_addr,
                             sizeof(server_addr));
    if (nr_sent != strlen(buf)) {
        printf("failed to send all bytes: %d\n", nr_sent);
    }

    printf("waiting for reply...\n");
    struct sockaddr_in remote_addr;
    int remote_size = sizeof(remote_addr);
    int nr_recv = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr *) &remote_addr,
                           &remote_size);
    if (nr_recv < 0) {
        printf("recvfrom() failed\n");
        return;
    }
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

    close(sock);

    return 0;
}
