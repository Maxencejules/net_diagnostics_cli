#ifndef NETDIAG_PING_H
#define NETDIAG_PING_H

// Run an OS-level ping and print a basic summary.
// count: number of echo requests (typical default 4). If <= 0, defaults to 4.
int ping_run_system(const char *host, int count);

#endif // NETDIAG_PING_H
