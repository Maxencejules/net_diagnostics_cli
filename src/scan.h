#ifndef NETDIAG_SCAN_H
#define NETDIAG_SCAN_H

#define SCAN_MAX_OPEN_PORTS 128

typedef struct {
    int status;                   // 0 ok, non-zero fail
    int start_port;
    int end_port;
    int open_count;
    int open_ports[SCAN_MAX_OPEN_PORTS];
} ScanResult;

// Runs a TCP connect scan. If print_live_output is 1, prints open ports as it finds them.
int tcp_scan(const char *host, int start_port, int end_port, int print_live_output, ScanResult *out);

#endif
