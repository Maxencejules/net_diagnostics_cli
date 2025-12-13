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
    int mn = -1, mx = -1, av = -1;
    if (sscanf(line, " Minimum = %dms, Maximum = %dms, Average = %dms", &mn, &mx, &av) == 3) {
        *min_ms = mn;
        *max_ms = mx;
        *avg_ms = av;
        return 1;
    }
    return 0;
}

int ping_run_system(const char *host, int count, int print_live_output, PingResult *out) {
    if (!out) return 1;

    out->exit_code = 1;
    out->loss_percent = -1;
    out->min_ms = -1;
    out->max_ms = -1;
    out->avg_ms = -1;

    if (!host || host[0] == '\0') {
        return 1;
    }

    if (count <= 0) count = 4;

    char cmd[512];

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "ping -n %d %s", count, host);
#else
    snprintf(cmd, sizeof(cmd), "ping -c %d %s", count, host);
#endif

    if (print_live_output) {
        printf("Running: %s\n\n", cmd);
    }

    FILE *pipe = POPEN(cmd, "r");
    if (!pipe) {
        return 1;
    }

    char line[1024];

    while (fgets(line, sizeof(line), pipe)) {
        if (print_live_output) {
            fputs(line, stdout);
        }

#ifdef _WIN32
        if (out->loss_percent < 0) {
            (void)parse_windows_loss_percent(line, &out->loss_percent);
        }
        if (out->avg_ms < 0) {
            (void)parse_windows_rtt_line(line, &out->min_ms, &out->max_ms, &out->avg_ms);
        }
#endif
    }

    int rc = PCLOSE(pipe);

    // Normalize status
    out->exit_code = (rc == 0) ? 0 : rc;

    return out->exit_code == 0 ? 0 : 1;
}
