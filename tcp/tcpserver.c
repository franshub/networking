/* tcpserver.c */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    int sock = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(sock, 0) != 0) {
        printf("listen() failed\n");
        close(sock);
        return -1;
    }

    printf("socket bound to port %d\n", port);
    return sock;
}

void handle_incoming(int sock) {
    struct sockaddr_in remote_addr;
    socklen_t remote_size = sizeof(remote_addr);

    printf("accepting incoming tcp connections...\n");
    int csock = accept(sock, (struct sockaddr *) &remote_addr, &remote_size);
    if (csock < 0) {
        printf("accept() failed\n");
        close(sock);
        return;
    }

    char addr_s[64];
    sa_tostring((struct sockaddr *) &remote_addr, addr_s);
    printf("connected with %s\n", addr_s);

    char data[256];
    sprintf(data, "Hello %s\n", addr_s);
    ssize_t nr_sent = send(csock, data, strlen(data), 0);
    if (nr_sent != strlen(data)) {
        printf("failed to send all bytes: %d\n", nr_sent);
    }

    close(csock);
}

int main(int argc, char *argv[]) {
    uint16_t port = 5000;
    int sock = create_socket(port);

    handle_incoming(sock);

    close(sock);

    return 0;
}
