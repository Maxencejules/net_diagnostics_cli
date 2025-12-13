# netdiag — Network Diagnostics CLI (C)

netdiag is a cross-platform command-line network diagnostics tool written in C.
It is designed to resemble tooling used by developer support, infrastructure,
and systems engineers for troubleshooting connectivity issues.

## Features

- DNS resolution (IPv4 / IPv6)
- ICMP ping using system ping
- TCP port scanning
- Aggregated diagnostic report
- JSON output for automation and debugging
- Cross-platform (Windows, Linux, macOS)

## Commands

### Ping

Send ICMP echo requests to a host.

- .\cmake-build-debug\netdiag.exe ping google.com
- .\cmake-build-debug\netdiag.exe ping google.com 4
- .\cmake-build-debug\netdiag.exe ping google.com 4 --json

### DNS

Resolve DNS records for a hostname.

- .\cmake-build-debug\netdiag.exe dns google.com
- .\cmake-build-debug\netdiag.exe dns google.com --json

### Scan

Scan a range of TCP ports.

- .\cmake-build-debug\netdiag.exe scan google.com
- .\cmake-build-debug\netdiag.exe scan google.com 80 443
- .\cmake-build-debug\netdiag.exe scan google.com 80 443 --json

### Report

Run DNS, ping, and scan together.

- .\cmake-build-debug\netdiag.exe report google.com
- .\cmake-build-debug\netdiag.exe report google.com --json

Example output:

--- netdiag report ---

Host: google.com

[DNS]

[IPv4] 142.250.69.78

[Ping]

  loss: 0%

  rtt: min=14 max=17 avg=16 ms

[Scan]

  TCP 80 open

  TCP 443 open

Status: ok

## Build Instructions

Requirements:
- CMake 3.20+
- C compiler (GCC, Clang, or MSVC)


## Design

- Modular architecture (dns, ping, scan, report)
- Structured result types
- Optional JSON output for scripting
- System ping for portability

## Future Improvements

- Traceroute implementation
- Health scoring
- Output to file
- Configurable report parameters


