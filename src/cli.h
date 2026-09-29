#ifndef NETDIAG_CLI_H
#define NETDIAG_CLI_H

typedef enum { CMD_HELP, CMD_PING, CMD_DNS, CMD_SCAN, CMD_REPORT } Command;
typedef struct {
    Command command;
    const char *host;
    int json;
    int count;
    int start_port;
    int end_port;
} CliOptions;

int cli_valid_host(const char *host);
int cli_parse(int argc, char *argv[], CliOptions *out, const char **error);
#endif
