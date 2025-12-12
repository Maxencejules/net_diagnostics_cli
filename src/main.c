#include <stdio.h>
#include <string.h>
#include "dns.h"
#include "scan.h"
#include <stdlib.h>



static void print_usage(const char *prog_name) {
    printf("netdiag - Network diagnostics CLI tool\n");
    printf("Usage:\n");
    printf("  %s <command> [options]\n", prog_name);
    printf("\n");
    printf("Commands:\n");
    printf("  ping   <host>       Send ICMP echo requests to a host (to be implemented)\n");
    printf("  trace  <host>       Trace network route to a host (to be implemented)\n");
    printf("  scan   <host>       Scan TCP ports on a host (to be implemented)\n");
    printf("  dns    <name>       Resolve DNS records for a name (to be implemented)\n");
    printf("  help                Show this message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s ping example.com\n", prog_name);
    printf("  %s trace 8.8.8.8\n", prog_name);
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(command, "ping") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: ping requires a host argument.\n\n");
            print_usage(argv[0]);
            return 1;
        }
        const char *host = argv[2];
        printf("[stub] Would run ICMP ping to host: %s\n", host);
        return 0;
    }

    if (strcmp(command, "trace") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: trace requires a host argument.\n\n");
            print_usage(argv[0]);
            return 1;
        }
        const char *host = argv[2];
        printf("[stub] Would run traceroute to host: %s\n", host);
        return 0;
    }

    if (strcmp(command, "scan") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: scan requires a host argument.\n\n");
            print_usage(argv[0]);
            return 1;
        }
        const char *host = argv[2];

        int start_port = 1;
        int end_port = 1024;

        if (argc >= 4) {
            start_port = atoi(argv[3]);
        }
        if (argc >= 5) {
            end_port = atoi(argv[4]);
        }

        return tcp_scan_range(host, start_port, end_port);
    }

    if (strcmp(command, "dns") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: dns requires a name argument.\n\n");
            print_usage(argv[0]);
            return 1;
        }
        const char *name = argv[2];
        return dns_resolve_and_print(name);
    }

    fprintf(stderr, "Unknown command: %s\n\n", command);
    print_usage(argv[0]);
    return 1;
}
