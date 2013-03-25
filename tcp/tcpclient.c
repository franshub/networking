/* tcpclient.c */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("socket() failed\n");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = in_addr;

    printf("connecting with %s:%d...\n", ipaddress, port);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
        printf("connect() failed\n");
        close(sock);
        return 0;
    }

    char data[256];
    printf("waiting for data...\n");
    ssize_t nr_recv = recv(sock, data, sizeof(data), 0);
    if (nr_recv < 0) {
        printf("recv() failed\n");
    } else if (nr_recv == 0) {
        printf("connection was closed by server\n");
    } else {
        printf("received %d bytes: '", nr_recv);
        int i;
        for (i = 0; i < nr_recv; i++) {
            if (isprint(data[i])) {
                putchar(data[i]);
            } else {
                printf("\\x%02.2x", data[i]);
            }
        }
        putchar('\'');
        putchar('\n');
    }

    printf("sending greeting to server...\n");
    sprintf(data, "Hi there\n");
    ssize_t nr_sent = send(sock, data, strlen(data), 0);
    if (nr_sent != strlen(data)) {
        printf("failed to send all bytes: %d\n", nr_sent);
    }

    close(sock);

    return 0;
}
