#ifndef NETDIAG_DNS_H
#define NETDIAG_DNS_H

#define DNS_MAX_RESULTS 32

typedef struct {
    char family[8];   // "IPv4" or "IPv6"
    char ip[64];
} DnsRecord;

typedef struct {
    int status;              // 0 ok, non-zero fail
    int record_count;
    DnsRecord records[DNS_MAX_RESULTS];
} DnsResult;

int dns_resolve(const char *name, DnsResult *out);

#endif
