#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dns.h"
#include "scan.h"
#include "ping.h"

static void print_usage(const char *prog_name) {
    printf("netdiag - Network diagnostics CLI tool\n");
    printf("Usage:\n");
    printf("  %s <command> [options]\n", prog_name);
    printf("\n");
    printf("Commands:\n");
    printf("  ping   <host> [count] [--json]      Send ICMP echo requests using system ping\n");
    printf("  trace  <host>                       Trace network route to a host (to be implemented)\n");
    printf("  scan   <host> [start] [end]         Scan TCP ports on a host (default 1-1024)\n");
    printf("  dns    <name>                       Resolve DNS records for a name\n");
    printf("  help                                Show this message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s ping google.com 4\n", prog_name);
    printf("  %s ping google.com 4 --json\n", prog_name);
    printf("  %s scan example.com 80 90\n", prog_name);
    printf("  %s dns google.com\n", prog_name);
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
        int count = 4;
        int json = 0;

        // parse extra args: [count] [--json]
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--json") == 0) {
                json = 1;
            } else {
                // assume it's count
                count = atoi(argv[i]);
            }
        }

        PingResult r;
        int ok = ping_run_system(host, count, json ? 0 : 1, &r);

        if (json) {
            printf("{\"command\":\"ping\",\"host\":\"%s\",\"count\":%d,", host, count);
            printf("\"status\":\"%s\",", ok == 0 ? "ok" : "failed");
            printf("\"lossPercent\":%d,", r.loss_percent);
            printf("\"rttMs\":{\"min\":%d,\"max\":%d,\"avg\":%d}}\n", r.min_ms, r.max_ms, r.avg_ms);
        } else {
            printf("\n--- netdiag ping summary ---\n");
            printf("host: %s\n", host);
            printf("count: %d\n", count);

            if (r.loss_percent >= 0) printf("loss_percent: %d\n", r.loss_percent);
            else printf("loss_percent: (not parsed)\n");

            if (r.avg_ms >= 0) printf("rtt_ms: min=%d max=%d avg=%d\n", r.min_ms, r.max_ms, r.avg_ms);
            else printf("rtt_ms: (not parsed)\n");

            printf("status: %s\n", ok == 0 ? "ok" : "failed");
        }

        return ok;
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
        int json = 0;

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--json") == 0) {
                json = 1;
            }
        }

        DnsResult r;
        int ok = dns_resolve(name, &r);

        if (json) {
            printf("{\"command\":\"dns\",\"name\":\"%s\",", name);
            printf("\"status\":\"%s\",", ok == 0 ? "ok" : "failed");
            printf("\"records\":[");

            for (int i = 0; i < r.record_count; i++) {
                printf("{\"family\":\"%s\",\"ip\":\"%s\"}",
                       r.records[i].family,
                       r.records[i].ip);
                if (i + 1 < r.record_count) printf(",");
            }

            printf("]}\n");
        } else {
            printf("DNS results for %s:\n", name);

            if (r.record_count == 0) {
                printf("  (no records found)\n");
            }

            for (int i = 0; i < r.record_count; i++) {
                printf("  [%s] %s\n",
                       r.records[i].family,
                       r.records[i].ip);
            }
        }

        return ok;
    }


    fprintf(stderr, "Unknown command: %s\n\n", command);
    print_usage(argv[0]);
    return 1;
}
