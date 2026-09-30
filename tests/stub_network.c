#include "dns.h"
#include "ping.h"
#include "scan.h"
#include <string.h>

/* Deterministic command/output contract fixtures; no sockets or child processes. */
int dns_resolve(const char *host, DnsResult *out) {
    memset(out, 0, sizeof(*out));
    out->status = !strcmp(host, "failure.test");
    return out->status;
}

int ping_run_system(const char *host, int count, int live, PingResult *out) {
    (void)count; (void)live;
    memset(out, 0, sizeof(*out));
    out->min_ms = 1.125;
    out->max_ms = 2.25;
    out->avg_ms = 1.75;
    if (!strcmp(host, "unknown.test") || !strcmp(host, "failure.test"))
        out->loss_percent = out->min_ms = out->max_ms = out->avg_ms = -1;
    out->exit_code = !strcmp(host, "failure.test");
    return out->exit_code;
}

int tcp_scan(const char *host, int first, int last, int live, ScanResult *out) {
    (void)live;
    memset(out, 0, sizeof(*out));
    out->start_port = first;
    out->end_port = last;
    out->status = !strcmp(host, "failure.test");
    return out->status;
}
