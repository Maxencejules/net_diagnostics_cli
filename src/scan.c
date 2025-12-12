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

// Set socket to non-blocking mode
static int set_nonblocking_socket(int sock) {
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
        return -1;
    }
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
#endif
    return 0;
}

int tcp_scan_range(const char *host, int start_port, int end_port) {
    if (start_port < 1 || end_port > 65535 || start_port > end_port) {
        fprintf(stderr, "Invalid port range: %d-%d\n", start_port, end_port);
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
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
    int ret = getaddrinfo(host, NULL, &hints, &res);
    if (ret != 0 || res == NULL) {
#ifdef _WIN32
        fprintf(stderr, "getaddrinfo failed for '%s': %d\n", host, WSAGetLastError());
        WSACleanup();
#else
        fprintf(stderr, "getaddrinfo failed for '%s': %s\n", host, gai_strerror(ret));
#endif
        return 1;
    }

    // Choose the first usable address
    struct addrinfo *ai = res;
    while (ai && !(ai->ai_family == AF_INET || ai->ai_family == AF_INET6)) {
        ai = ai->ai_next;
    }

    if (!ai) {
        fprintf(stderr, "No usable address found for %s\n", host);
        freeaddrinfo(res);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    char addr_str[INET6_ADDRSTRLEN];
    void *addr_ptr = NULL;
    if (ai->ai_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *) ai->ai_addr;
        addr_ptr = &ipv4->sin_addr;
    } else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) ai->ai_addr;
        addr_ptr = &ipv6->sin6_addr;
    }
    inet_ntop(ai->ai_family, addr_ptr, addr_str, sizeof(addr_str));

    printf("TCP scan on %s (%s) ports %d-%d\n", host, addr_str, start_port, end_port);

    // Copy base address into a modifiable struct
    struct sockaddr_storage base_addr;
    memset(&base_addr, 0, sizeof(base_addr));
    memcpy(&base_addr, ai->ai_addr, ai->ai_addrlen);
    socklen_t addr_len = (socklen_t)ai->ai_addrlen;

    freeaddrinfo(res);

    int open_count = 0;

    for (int port = start_port; port <= end_port; ++port) {
        int sock = (int)socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            continue;
        }

        // Set port for this attempt
        if (ai->ai_family == AF_INET) {
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
            // Connected immediately
            printf("  [OPEN]  TCP %d\n", port);
            open_count++;
            CLOSESOCK(sock);
            continue;
        }

        // On non-blocking sockets, connect will "fail" with in-progress
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS && err != WSAEALREADY) {
            CLOSESOCK(sock);
            continue;
        }
#else
        // POSIX: EINPROGRESS is expected
        // We don't check errno here; select will tell us
#endif

        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 300000; // 300ms timeout

        int sret = select(sock + 1, NULL, &writefds, NULL, &tv);
        if (sret > 0 && FD_ISSET(sock, &writefds)) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&so_error, &len);
            if (so_error == 0) {
                printf("  [OPEN]  TCP %d\n", port);
                open_count++;
            }
        }

        CLOSESOCK(sock);
    }

    if (open_count == 0) {
        printf("No open TCP ports found in range.\n");
    } else {
        printf("Total open ports: %d\n", open_count);
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
