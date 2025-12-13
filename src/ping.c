#include "ping.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define POPEN _popen
  #define PCLOSE _pclose
#else
  #define POPEN popen
  #define PCLOSE pclose
#endif

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

int ping_run_system(const char *host, int count) {
    if (!host || host[0] == '\0') {
        fprintf(stderr, "ping: host is required\n");
        return 1;
    }

    if (count <= 0) count = 4;

    char cmd[512];

#ifdef _WIN32
    // Windows: -n <count>
    // Add -w <timeout_ms> later if needed
    snprintf(cmd, sizeof(cmd), "ping -n %d %s", count, host);
#else
    // Linux/macOS: -c <count>
    snprintf(cmd, sizeof(cmd), "ping -c %d %s", count, host);
#endif

    printf("Running: %s\n\n", cmd);

    FILE *pipe = POPEN(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "ping: failed to start command\n");
        return 1;
    }

    char line[1024];
    int seen_loss = 0;
    int seen_rtt = 0;

    // We'll print the live output (good for debugging) and also extract summary lines.
    while (fgets(line, sizeof(line), pipe)) {
        fputs(line, stdout);

        // Heuristic extraction:
        // Windows packet loss line often contains "Lost = X"
        // Linux/macOS summary contains "% packet loss"
        if (strstr(line, "Lost =") || strstr(line, "packet loss")) {
            seen_loss = 1;
        }

        // Linux/macOS RTT line often contains "min/avg/max"
        if (strstr(line, "min/avg") || strstr(line, "Average =")) {
            seen_rtt = 1;
        }
    }

    int rc = PCLOSE(pipe);

    printf("\n--- netdiag ping summary ---\n");
    printf("host: %s\n", host);
    printf("count: %d\n", count);

    if (!seen_loss) {
        printf("loss: (not parsed)\n");
    } else {
        printf("loss: (see output above)\n");
    }

    if (!seen_rtt) {
        printf("rtt: (not parsed)\n");
    } else {
        printf("rtt: (see output above)\n");
    }

    // rc is command exit status. On many systems 0 means success.
    // We'll treat non-zero as "ping failed".
    if (rc != 0) {
        printf("status: failed (exit code %d)\n", rc);
        return 1;
    }

    printf("status: ok\n");
    return 0;
}
