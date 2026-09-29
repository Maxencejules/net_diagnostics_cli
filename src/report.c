#include "report.h"

#include <string.h>
#include <stdio.h>

int run_report(
    const char *host,
    int ping_count,
    int scan_start,
    int scan_end,
    ReportResult *out
) {
    if (!host || !out) return 1;

    memset(out, 0, sizeof(*out));
    out->host = host;
    out->status = 0;

    if (dns_resolve(host, &out->dns) != 0) {
        out->status = 1;
    }

    if (ping_run_system(host, ping_count, 0, &out->ping) != 0) {
        out->status = 1;
    }

    if (tcp_scan(host, scan_start, scan_end, 0, &out->scan) != 0) {
        out->status = 1;
    }

    return out->status;
}
