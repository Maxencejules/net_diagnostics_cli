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

static int parse_windows_loss_percent(const char *line, int *loss_percent) {
    // Example:
    // "    Packets: Sent = 4, Received = 4, Lost = 0 (0% loss),"
    const char *p = strchr(line, '(');
    if (!p) return 0;
    int lp = -1;
    if (sscanf(p, "(%d%% loss", &lp) == 1 && lp >= 0) {
        *loss_percent = lp;
        return 1;
    }
    return 0;
}

static int parse_windows_rtt_line(const char *line, int *min_ms, int *max_ms, int *avg_ms) {
    // Example:
    // "    Minimum = 16ms, Maximum = 19ms, Average = 17ms"
    int mn = -1, mx = -1, av = -1;
    if (sscanf(line, " Minimum = %dms, Maximum = %dms, Average = %dms", &mn, &mx, &av) == 3) {
        *min_ms = mn;
        *max_ms = mx;
        *avg_ms = av;
        return 1;
    }
    return 0;
}

int ping_run_system(const char *host, int count) {
    if (!host || host[0] == '\0') {
        fprintf(stderr, "ping: host is required\n");
        return 1;
    }

    if (count <= 0) count = 4;

    char cmd[512];

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "ping -n %d %s", count, host);
#else
    snprintf(cmd, sizeof(cmd), "ping -c %d %s", count, host);
#endif

    printf("Running: %s\n\n", cmd);

    FILE *pipe = POPEN(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "ping: failed to start command\n");
        return 1;
    }

    char line[1024];

    // Parsed metrics (when available)
    int loss_percent = -1;
    int min_ms = -1, max_ms = -1, avg_ms = -1;

    while (fgets(line, sizeof(line), pipe)) {
        fputs(line, stdout);

#ifdef _WIN32
        if (loss_percent < 0) {
            (void)parse_windows_loss_percent(line, &loss_percent);
        }
        if (avg_ms < 0) {
            (void)parse_windows_rtt_line(line, &min_ms, &max_ms, &avg_ms);
        }
#endif
    }

    int rc = PCLOSE(pipe);

    printf("\n--- netdiag ping summary ---\n");
    printf("host: %s\n", host);
    printf("count: %d\n", count);

#ifdef _WIN32
    if (loss_percent >= 0) {
        printf("loss_percent: %d\n", loss_percent);
    } else {
        printf("loss_percent: (not parsed)\n");
    }

    if (avg_ms >= 0) {
        printf("rtt_ms: min=%d max=%d avg=%d\n", min_ms, max_ms, avg_ms);
    } else {
        printf("rtt_ms: (not parsed)\n");
    }
#else
    // We will add Linux/macOS parsing next step.
    printf("loss_percent: (not parsed)\n");
    printf("rtt_ms: (not parsed)\n");
#endif

    if (rc != 0) {
        printf("status: failed (exit code %d)\n", rc);
        return 1;
    }

    printf("status: ok\n");
    return 0;
}
