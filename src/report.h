#ifndef NETDIAG_REPORT_H
#define NETDIAG_REPORT_H

#include "dns.h"
#include "ping.h"
#include "scan.h"

typedef struct {
    int status;   // 0 ok, non-zero fail
    const char *host;

    DnsResult dns;
    PingResult ping;
    ScanResult scan;
} ReportResult;

int run_report(
    const char *host,
    int ping_count,
    int scan_start,
    int scan_end,
    ReportResult *out
);

#endif
