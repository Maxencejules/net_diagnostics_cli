#include "ping.h"
#include "cli.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#include <sys/wait.h>
#define POPEN popen
#define PCLOSE pclose
#endif

void ping_result_init(PingResult *out) {
    *out = (PingResult){ .exit_code = -1, .loss_percent = -1, .min_ms = -1, .max_ms = -1, .avg_ms = -1 };
}

static int valid_rtt(double min, double max, double avg) {
    return isfinite(min) && isfinite(max) && isfinite(avg) && min >= 0 && min <= avg && avg <= max;
}

int ping_parse_line(const char *line, PingResult *out) {
    int parsed = 0, consumed = 0;
    double loss, min, max, avg, deviation;
    const char *p = strchr(line, '(');
    if (p && sscanf(p, "(%lf%% loss)%n", &loss, &consumed) == 1 && consumed > 0 &&
        isfinite(loss) && loss >= 0 && loss <= 100) {
        out->loss_percent = loss;
        parsed = 1;
    }
    p = strstr(line, " packet loss");
    if (p && p > line && p[-1] == '%') {
        const char *end = p - 1;
        const char *start = end;
        while (start > line && !isspace((unsigned char)start[-1]) && start[-1] != ',') --start;
        char *after;
        loss = strtod(start, &after);
        if (after == end && start < end && isfinite(loss) && loss >= 0 && loss <= 100) {
            out->loss_percent = loss;
            parsed = 1;
        }
    }
    p = strstr(line, "Minimum =");
    consumed = 0;
    if (p && sscanf(p, "Minimum = %lfms, Maximum = %lfms, Average = %lfms%n", &min, &max, &avg, &consumed) == 3 &&
        consumed > 0 && valid_rtt(min, max, avg)) {
        out->min_ms = min; out->max_ms = max; out->avg_ms = avg;
        parsed = 1;
    }
    p = strchr(line, '=');
    consumed = 0;
    if (p && strstr(line, "min/avg/max") &&
        sscanf(p + 1, " %lf/%lf/%lf/%lf ms%n", &min, &avg, &max, &deviation, &consumed) == 4 &&
        consumed > 0 && valid_rtt(min, max, avg) && isfinite(deviation) && deviation >= 0) {
        out->min_ms = min; out->max_ms = max; out->avg_ms = avg;
        parsed = 1;
    }
    return parsed;
}

int ping_run_system(const char *host, int count, int print_live_output, PingResult *out) {
    if (!out) return 1;
    ping_result_init(out);
    if (!cli_valid_host(host) || count < 1 || count > 100) return 1;
    char cmd[512];
#ifdef _WIN32
    int length = snprintf(cmd, sizeof(cmd), "ping -n %d \"%s\" 2>&1", count, host);
#else
#ifdef __APPLE__
    const char *program = strchr(host, ':') ? "ping6" : "ping";
#else
    const char *program = "ping";
#endif
    int length = snprintf(cmd, sizeof(cmd), "LC_ALL=C %s -c %d '%s' 2>&1", program, count, host);
#endif
    if (length < 0 || (size_t)length >= sizeof(cmd)) return 1;
    FILE *pipe = POPEN(cmd, "r");
    if (!pipe) return 1;
    char line[1024];
    while (fgets(line, sizeof(line), pipe)) {
        if (print_live_output) fputs(line, stdout);
        (void)ping_parse_line(line, out);
    }
    int rc = PCLOSE(pipe);
#ifndef _WIN32
    if (rc != -1) rc = WIFEXITED(rc) ? WEXITSTATUS(rc) : (WIFSIGNALED(rc) ? 128 + WTERMSIG(rc) : 1);
#endif
    out->exit_code = rc;
    return rc == 0 ? 0 : 1;
}
