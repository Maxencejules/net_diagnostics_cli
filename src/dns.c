#include "dns.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <arpa/inet.h>
#endif

int dns_resolve(const char *name, DnsResult *out) {
    if (!out) return 1;
    memset(out, 0, sizeof(*out));
    out->status = 1;
    if (!name || !*name) return 1;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        return 1;
    }
#endif

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    int r = getaddrinfo(name, NULL, &hints, &res);

    if (r != 0 || !res) {
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int count = 0;

    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        char buf[INET6_ADDRSTRLEN];
        const char *family;
        if (ai->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)ai->ai_addr;
            if (!inet_ntop(AF_INET, &ipv4->sin_addr, buf, sizeof(buf))) continue;
            family = "IPv4";
        } else if (ai->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ai->ai_addr;
            if (!inet_ntop(AF_INET6, &ipv6->sin6_addr, buf, sizeof(buf))) continue;
            family = "IPv6";
        } else continue;
        int duplicate = 0;
        for (int i = 0; i < count; ++i)
            if (!strcmp(out->records[i].family, family) && !strcmp(out->records[i].ip, buf)) duplicate = 1;
        if (duplicate) continue;
        if (count == DNS_MAX_RESULTS) { out->truncated = 1; continue; }
        strcpy(out->records[count].family, family);
        strcpy(out->records[count++].ip, buf);
    }

    freeaddrinfo(res);

#ifdef _WIN32
    WSACleanup();
#endif

    out->record_count = count;
    out->status = count ? 0 : 1;
    return out->status;
}
