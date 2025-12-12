#ifndef NETDIAG_SCAN_H
#define NETDIAG_SCAN_H

// Scan TCP ports on a host between start_port and end_port (inclusive).
// Returns 0 on success, non-zero on error.
int tcp_scan_range(const char *host, int start_port, int end_port);

#endif // NETDIAG_SCAN_H
