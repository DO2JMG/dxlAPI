#define _POSIX_C_SOURCE 200809L
#include "udp.h"
#include "log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int create_udp_socket(const char *bind_addr, int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        log_msg("ERROR", "socket failed: %s", strerror(errno));
        return -1;
    }

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, bind_addr, &addr.sin_addr) != 1) {
        log_msg("ERROR", "Invalid bind address: %s", bind_addr);
        close(sock);
        return -1;
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_msg("ERROR", "bind %s:%d failed: %s", bind_addr, port, strerror(errno));
        close(sock);
        return -1;
    }
    return sock;
}
