#ifndef NETDIAG_PING_H
#define NETDIAG_PING_H

typedef struct PingResult {
    int exit_code;      // Normalized OS exit code; -1 if unavailable.
    double loss_percent;   // -1 if unknown
    double min_ms;         // -1 if unknown
    double max_ms;         // -1 if unknown
    double avg_ms;         // -1 if unknown
} PingResult;

// Run ping and fill result. If print_live_output is 1, prints raw ping output.
int ping_run_system(const char *host, int count, int print_live_output, PingResult *out);
void ping_result_init(PingResult *out);
// English Windows and C-locale Linux/macOS summary lines; returns 1 if parsed.
int ping_parse_line(const char *line, PingResult *out);

#endif
