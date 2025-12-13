#include "scan.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define CLOSESOCK closesocket
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #define CLOSESOCK close
#endif

static int set_nonblocking_socket(int sock) {
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode) != 0) return -1;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
#endif
    return 0;
}

int tcp_scan(const char *host, int start_port, int end_port, int print_live_output, ScanResult *out) {
    if (!host || !out) return 1;

    memset(out, 0, sizeof(*out));
    out->status = 1;
    out->start_port = start_port;
    out->end_port = end_port;

    if (start_port < 1 || end_port > 65535 || start_port > end_port) {
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        return 1;
    }
#endif

    // Resolve host once
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *res = NULL;
    int r = getaddrinfo(host, NULL, &hints, &res);
    if (r != 0 || res == NULL) {
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // choose first IPv4/IPv6 address
    struct addrinfo *ai = res;
    while (ai && !(ai->ai_family == AF_INET || ai->ai_family == AF_INET6)) {
        ai = ai->ai_next;
    }
    if (!ai) {
        freeaddrinfo(res);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // store base address
    struct sockaddr_storage base_addr;
    memset(&base_addr, 0, sizeof(base_addr));
    memcpy(&base_addr, ai->ai_addr, ai->ai_addrlen);
    socklen_t addr_len = (socklen_t)ai->ai_addrlen;

    int family = ai->ai_family;

    freeaddrinfo(res);

    if (print_live_output) {
        printf("TCP scan on %s ports %d-%d\n", host, start_port, end_port);
    }

    for (int port = start_port; port <= end_port; ++port) {
        int sock = (int)socket(family, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) continue;

        // set port
        if (family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)&base_addr;
            ipv4->sin_port = htons((unsigned short)port);
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&base_addr;
            ipv6->sin6_port = htons((unsigned short)port);
        }

        if (set_nonblocking_socket(sock) != 0) {
            CLOSESOCK(sock);
            continue;
        }

        int c = connect(sock, (struct sockaddr *)&base_addr, addr_len);

        if (c == 0) {
            // open immediately
            if (out->open_count < SCAN_MAX_OPEN_PORTS) {
                out->open_ports[out->open_count++] = port;
            }
            if (print_live_output) {
                printf("  [OPEN] TCP %d\n", port);
            }
            CLOSESOCK(sock);
            continue;
        }

#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS && err != WSAEALREADY) {
            CLOSESOCK(sock);
            continue;
        }
#endif

        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 300000; // 300ms

        int sret = select(sock + 1, NULL, &writefds, NULL, &tv);
        if (sret > 0 && FD_ISSET(sock, &writefds)) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&so_error, &len);
            if (so_error == 0) {
                if (out->open_count < SCAN_MAX_OPEN_PORTS) {
                    out->open_ports[out->open_count++] = port;
                }
                if (print_live_output) {
                    printf("  [OPEN] TCP %d\n", port);
                }
            }
        }

        CLOSESOCK(sock);
    }

    out->status = 0;

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
