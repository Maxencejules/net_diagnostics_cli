#include "cli.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int cli_valid_host(const char *host) {
    if (!host || !*host || *host == '-' || strlen(host) > 253) return 0;
    /* ASCII hostnames and unbracketed IP literals only; no shell metacharacters. */
    for (const unsigned char *p = (const unsigned char *)host; *p; ++p) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || strchr(".-_:", *p))) return 0;
    }
    return 1;
}

static int number(const char *text, int max, int *out) {
    if (!*text) return 0;
    for (const char *p = text; *p; ++p) if (*p < '0' || *p > '9') return 0;
    errno = 0;
    char *end;
    long value = strtol(text, &end, 10);
    if (errno || *end || value < 1 || value > max) return 0;
    *out = (int)value;
    return 1;
}

int cli_parse(int argc, char *argv[], CliOptions *out, const char **error) {
    *out = (CliOptions){ .command = CMD_HELP, .count = 4, .start_port = 1, .end_port = 1024 };
    *error = "Invalid arguments. Use 'netdiag help' for usage.";
    if (argc < 2) return 0;
    if (!strcmp(argv[1], "help") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))
        return argc == 2;
    if (!strcmp(argv[1], "ping")) out->command = CMD_PING;
    else if (!strcmp(argv[1], "dns")) out->command = CMD_DNS;
    else if (!strcmp(argv[1], "scan")) out->command = CMD_SCAN;
    else if (!strcmp(argv[1], "report")) out->command = CMD_REPORT;
    else { *error = "Unknown command. Use 'netdiag help' for usage."; return 0; }
    if (argc < 3 || !cli_valid_host(argv[2])) {
        *error = "A host is required: use an ASCII hostname or unbracketed IPv4/IPv6 address.";
        return 0;
    }
    out->host = argv[2];
    const char *values[3];
    int count = 0;
    for (int i = 3; i < argc; ++i) {
        if (!strcmp(argv[i], "--json")) {
            if (out->json) return 0;
            out->json = 1;
        } else {
            if (count == 3) return 0;
            values[count++] = argv[i];
        }
    }
    if (out->command == CMD_DNS) return count == 0;
    if (out->command == CMD_PING)
        return count == 0 || (count == 1 && number(values[0], 100, &out->count));
    if (out->command == CMD_SCAN) {
        if (count > 2) return 0;
        if (count && !number(values[0], 65535, &out->start_port)) return 0;
        if (count == 1) out->end_port = out->start_port;
        if (count == 2 && !number(values[1], 65535, &out->end_port)) return 0;
    } else if (count) {
        if (count != 3 || !number(values[0], 100, &out->count) ||
            !number(values[1], 65535, &out->start_port) ||
            !number(values[2], 65535, &out->end_port)) return 0;
    }
    return out->start_port <= out->end_port;
}
