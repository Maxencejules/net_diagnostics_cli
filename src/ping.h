#ifndef NETDIAG_PING_H
#define NETDIAG_PING_H

typedef struct PingResult {
    int exit_code;      // 0 ok, non-zero fail
    int loss_percent;   // -1 if unknown
    int min_ms;         // -1 if unknown
    int max_ms;         // -1 if unknown
    int avg_ms;         // -1 if unknown
} PingResult;

// Run ping and fill result. If print_live_output is 1, prints raw ping output.
int ping_run_system(const char *host, int count, int print_live_output, PingResult *out);

#endif
