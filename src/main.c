#include <stdio.h>
#include "cli.h"
#include "json.h"
#include "report.h"

static void usage(const char *program) {
    printf("netdiag - Network diagnostics CLI\nUsage:\n"
           "  %s ping <host> [count] [--json]\n"
           "  %s dns <host> [--json]\n"
           "  %s scan <host> [start [end]] [--json]\n"
           "  %s report <host> [count start end] [--json]\n"
           "  %s help\n\n"
           "Defaults: count=4, ports=1-1024. One scan port scans that port only.\n"
           "Count: 1-100; ports: 1-65535. --json may follow the host in any position.\n",
           program, program, program, program, program);
}

static const char *status(int code) { return code == 0 ? "ok" : "failed"; }

static void dns_json(const DnsResult *r) {
    printf("\"status\":\"%s\",\"records\":[", status(r->status));
    for (int i = 0; i < r->record_count; ++i) {
        if (i) putchar(',');
        fputs("{\"family\":", stdout); json_string(stdout, r->records[i].family);
        fputs(",\"ip\":", stdout); json_string(stdout, r->records[i].ip);
        putchar('}');
    }
    printf("],\"truncated\":%s", r->truncated ? "true" : "false");
}

static void ping_json(const PingResult *r, int count) {
    printf("\"count\":%d,\"status\":\"%s\",\"systemExitCode\":%d,\"lossPercent\":",
           count, status(r->exit_code), r->exit_code);
    json_metric(stdout, r->loss_percent);
    fputs(",\"rttMs\":{\"min\":", stdout); json_metric(stdout, r->min_ms);
    fputs(",\"max\":", stdout); json_metric(stdout, r->max_ms);
    fputs(",\"avg\":", stdout); json_metric(stdout, r->avg_ms);
    putchar('}');
}

static void scan_json(const ScanResult *r) {
    printf("\"startPort\":%d,\"endPort\":%d,\"status\":\"%s\",\"openPorts\":[",
           r->start_port, r->end_port, status(r->status));
    for (int i = 0; i < r->open_count; ++i) {
        if (i) putchar(',');
        printf("%d", r->open_ports[i]);
    }
    printf("],\"openPortCount\":%d,\"truncated\":%s,\"probeErrors\":%d",
           r->open_total, r->open_total > r->open_count ? "true" : "false", r->error_count);
}

static void dns_text(const DnsResult *r) {
    printf("DNS: %s\n", status(r->status));
    for (int i = 0; i < r->record_count; ++i)
        printf("  [%s] %s\n", r->records[i].family, r->records[i].ip);
    if (r->truncated) puts("  (address list truncated)");
}

static void metric_text(const char *label, double value) {
    if (value < 0) printf("%sunknown", label);
    else printf("%s%.3f", label, value);
}

static void ping_text(const PingResult *r) {
    printf("Ping: %s (system exit %d)\n", status(r->exit_code), r->exit_code);
    metric_text("  lossPercent=", r->loss_percent);
    metric_text(" rttMs min=", r->min_ms);
    metric_text(" max=", r->max_ms);
    metric_text(" avg=", r->avg_ms);
    putchar('\n');
}

static void scan_text(const ScanResult *r) {
    printf("Scan: %s, ports %d-%d, %d open, %d probe errors\n",
           status(r->status), r->start_port, r->end_port, r->open_total, r->error_count);
    for (int i = 0; i < r->open_count; ++i) printf("  TCP %d open\n", r->open_ports[i]);
    if (r->open_total > r->open_count) puts("  (open port list truncated)");
}

int main(int argc, char *argv[]) {
    CliOptions o;
    const char *error;
    if (!cli_parse(argc, argv, &o, &error)) { fprintf(stderr, "%s\n", error); return 2; }
    if (o.command == CMD_HELP) { usage(argv[0]); return 0; }
    int code;
    if (o.json) {
        const char *commands[] = { "help", "ping", "dns", "scan", "report" };
        printf("{\"schemaVersion\":1,\"command\":\"%s\",\"%s\":", commands[o.command],
               o.command == CMD_DNS ? "name" : "host");
        json_string(stdout, o.host);
        putchar(',');
    } else printf("Host: %s\n", o.host);
    switch (o.command) {
    case CMD_DNS: {
        DnsResult r;
        code = dns_resolve(o.host, &r);
        if (o.json) dns_json(&r); else dns_text(&r);
        break;
    }
    case CMD_PING: {
        PingResult r;
        code = ping_run_system(o.host, o.count, !o.json, &r);
        if (o.json) ping_json(&r, o.count); else ping_text(&r);
        break;
    }
    case CMD_SCAN: {
        ScanResult r;
        code = tcp_scan(o.host, o.start_port, o.end_port, !o.json, &r);
        if (o.json) scan_json(&r); else scan_text(&r);
        break;
    }
    case CMD_REPORT: {
        ReportResult r;
        code = run_report(o.host, o.count, o.start_port, o.end_port, &r);
        if (o.json) {
            fputs("\"dns\":{", stdout); dns_json(&r.dns);
            fputs("},\"ping\":{", stdout); ping_json(&r.ping, o.count);
            fputs("},\"scan\":{", stdout); scan_json(&r.scan);
            printf("},\"status\":\"%s\"", status(code));
        } else { dns_text(&r.dns); ping_text(&r.ping); scan_text(&r.scan); printf("Status: %s\n", status(code)); }
        break;
    }
    default: return 2;
    }
    if (o.json) puts("}");
    return code == 0 ? 0 : 1;
}
