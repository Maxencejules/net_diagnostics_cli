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

int dns_resolve_and_print(const char *name) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;

    struct addrinfo *res;
    int r = getaddrinfo(name, NULL, &hints, &res);

    if (r != 0) {
        fprintf(stderr, "DNS lookup failed: %s\n", gai_strerror(r));
        return 1;
    }

    printf("DNS results for %s:\n", name);
    int count = 0;

    for (struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
        char host[INET6_ADDRSTRLEN];

        if (ai->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)ai->ai_addr;
            inet_ntop(AF_INET, &ipv4->sin_addr, host, sizeof(host));
            printf("  [IPv4] %s\n", host);
            count++;
        } else if (ai->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ai->ai_addr;
            inet_ntop(AF_INET6, &ipv6->sin6_addr, host, sizeof(host));
            printf("  [IPv6] %s\n", host);
            count++;
        }
    }

    freeaddrinfo(res);

#ifdef _WIN32
    WSACleanup();
#endif

    if (count == 0)
        printf("  (no addresses found)\n");

    return 0;
}
