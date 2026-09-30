#include "scan.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
typedef int SocketLength;
#define CLOSESOCK closesocket
#define INVALID_SOCK INVALID_SOCKET
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int Socket;
typedef socklen_t SocketLength;
#define CLOSESOCK close
#define INVALID_SOCK (-1)
#endif

static int nonblocking(Socket sock) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    return flags != -1 && fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

static int connection_error(int error) {
#ifdef _WIN32
    return error == WSAECONNREFUSED || error == WSAETIMEDOUT || error == WSAENETUNREACH ||
           error == WSAEHOSTUNREACH || error == WSAECONNRESET ? 0 : -1;
#else
    return error == ECONNREFUSED || error == ETIMEDOUT || error == ENETUNREACH ||
           error == EHOSTUNREACH || error == ECONNRESET ? 0 : -1;
#endif
}

/* 1=open, 0=not observed open (refused/unreachable/timed out), -1=local probe error. */
static int probe(const struct addrinfo *ai, int port) {
    Socket sock = socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCK) return -1;
    if (!nonblocking(sock)) { CLOSESOCK(sock); return -1; }
#ifndef _WIN32
    if (sock >= FD_SETSIZE) { CLOSESOCK(sock); return -1; }
#endif
    struct sockaddr_storage address;
    if (ai->ai_addrlen > sizeof(address)) { CLOSESOCK(sock); return -1; }
    memset(&address, 0, sizeof(address));
    memcpy(&address, ai->ai_addr, ai->ai_addrlen);
    if (ai->ai_family == AF_INET) ((struct sockaddr_in *)&address)->sin_port = htons((unsigned short)port);
    else ((struct sockaddr_in6 *)&address)->sin6_port = htons((unsigned short)port);
    int result = connect(sock, (struct sockaddr *)&address, (SocketLength)ai->ai_addrlen);
    if (result == 0) { CLOSESOCK(sock); return 1; }
#ifdef _WIN32
    int error = WSAGetLastError();
    int pending = error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == WSAEALREADY;
#else
    int error = errno;
    int pending = error == EINPROGRESS || error == EWOULDBLOCK || error == EALREADY;
#endif
    if (!pending) { CLOSESOCK(sock); return connection_error(error); }
    fd_set writefds, exceptfds;
    FD_ZERO(&writefds); FD_ZERO(&exceptfds);
    FD_SET(sock, &writefds); FD_SET(sock, &exceptfds);
    struct timeval timeout = { 0, 300000 };
#ifdef _WIN32
    result = select(0, NULL, &writefds, &exceptfds, &timeout);
#else
    result = select(sock + 1, NULL, &writefds, &exceptfds, &timeout);
#endif
    if (result > 0) {
        int completion_error = 0;
        SocketLength length = (SocketLength)sizeof(completion_error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&completion_error, &length) != 0) result = -1;
        else result = completion_error == 0 ? 1 : connection_error(completion_error);
    } else if (result < 0) result = -1;
    CLOSESOCK(sock);
    return result;
}

int tcp_scan(const char *host, int start_port, int end_port, int live, ScanResult *out) {
    if (!out) return 1;
    memset(out, 0, sizeof(*out));
    out->status = 1;
    out->start_port = start_port;
    out->end_port = end_port;
    if (!host || !*host || start_port < 1 || end_port > 65535 || start_port > end_port) return 1;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
#endif
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo *addresses = NULL;
    int resolved = getaddrinfo(host, NULL, &hints, &addresses);
    if (!resolved && addresses) {
        int supported = 0;
        for (struct addrinfo *ai = addresses; ai; ai = ai->ai_next)
            if (ai->ai_family == AF_INET || ai->ai_family == AF_INET6) supported = 1;
        if (supported) {
            for (int port = start_port; port <= end_port; ++port) {
                int open = 0;
                for (struct addrinfo *ai = addresses; ai; ai = ai->ai_next) {
                    if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6) continue;
                    int result = probe(ai, port);
                    if (result < 0) ++out->error_count;
                    if (result == 1) { open = 1; break; }
                }
                if (open) {
                    ++out->open_total;
                    if (out->open_count < SCAN_MAX_OPEN_PORTS) out->open_ports[out->open_count++] = port;
                    if (live) printf("  [OPEN] TCP %d\n", port);
                }
            }
            out->status = out->error_count ? 1 : 0;
        }
    }
    if (addresses) freeaddrinfo(addresses);
#ifdef _WIN32
    WSACleanup();
#endif
    return out->status;
}
