# netdiag — Network Diagnostics CLI

A compact C11 tool for resolving addresses, reading system ping summaries, and checking TCP connectivity. It runs on Windows, Linux, and macOS using native sockets and the OS `ping` command. The diagnostic executable has no third-party library dependencies.

## Build and verify

Requirements: CMake 3.20+, a C11 compiler (GCC, Clang, or recent MSVC), and Python 3.9+ for tests/demo. Install the OS ping utility if it is absent (`iputils-ping` on Debian/Ubuntu). The build does not download packages.

Linux/macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/netdiag dns localhost --json
python3 scripts/demo.py
```

Windows with Visual Studio's C tools:

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
.\build\Release\netdiag.exe dns localhost --json
python scripts/demo.py
```

For MinGW, configure with `-G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release`; the executable is `build/netdiag.exe`. To build only the diagnostic tool without Python, configure with `-DBUILD_TESTING=OFF`.

The demo starts a loopback HTTP `/health` endpoint on an available port, verifies its response, then runs a one-packet report scanning only that port. It requires no external network target. The HTTP health check belongs to the demo; netdiag itself checks TCP connectivity rather than HTTP application health.

## Commands

```text
netdiag ping <host> [count] [--json]
netdiag dns <host> [--json]
netdiag scan <host> [start [end]] [--json]
netdiag report <host> [count start end] [--json]
netdiag help
```

- Ping defaults to 4 packets; accepted counts are 1–100.
- Scan defaults to ports 1–1024. One supplied port scans that port only; two supply an inclusive range (1–65535, start ≤ end).
- Report runs DNS, ping, and scan. Supply all three numeric values to use a bounded custom report, for example `report 127.0.0.1 1 8000 8000 --json`.
- `--json` may appear anywhere after the host. Unknown options, duplicate flags, extra arguments, malformed numbers, and overflow are rejected before network operations.
- Hosts accept ASCII letters, digits, dots, hyphens, underscores, and colons, up to 253 characters; a leading hyphen is rejected. The resolver determines whether an allowed token actually names a host. Use unbracketed IPv6 literals (`::1`). IPv6 scope identifiers (`%eth0`/`%3`) and Unicode IDNs are unsupported; convert IDNs to ASCII punycode yourself. On macOS an IPv6 literal uses `ping6`.

Scan tests all resolved IPv4/IPv6 addresses until one connects for each port. It resolves the host once and scans sequentially, with up to 300 ms per address/port. Large ranges and filtered endpoints can take minutes; prefer a small explicit range.

## JSON and exit contract

For a valid command with `--json`, stdout contains exactly one JSON object and a newline, including operational failures. Usage errors write to stderr, produce no stdout, and exit with **2**. Diagnostics exit with **0** when the requested operations complete successfully, or **1** when an operation fails. `help` exits with 0.

```json
{"schemaVersion":1,"command":"ping","host":"127.0.0.1","count":1,"status":"ok","systemExitCode":0,"lossPercent":0,"rttMs":{"min":0.027,"max":0.072,"avg":0.042}}
```

This example illustrates the contract; measurements depend on the OS and target.

| Field | Meaning |
| --- | --- |
| `schemaVersion` | Integer `1`, present on each top-level diagnostic object. |
| `command`, `host` | Command and target; DNS uses `name` instead of `host`. |
| `status` | `ok` or `failed`, describing operation completion. Report is `failed` if any component fails. |
| DNS `records` | Array of unique `{family: "IPv4"\|"IPv6", ip: string}` addresses. At most 32 are stored; `truncated` reports omitted addresses. This is host address resolution, not an arbitrary DNS record query. |
| Ping `count`, `systemExitCode` | Requested packet count and normalized OS exit code. POSIX wait status is decoded; signal termination is `128 + signal`. `-1` means the command could not be started/reaped. A missing utility normally yields a nonzero shell exit code. |
| Ping `lossPercent`, `rttMs` | Numbers or `null` when unavailable/unrecognized. RTT contains `min`, `max`, `avg` in milliseconds and preserves fractional values. Loss ranges from 0 to 100. |
| Scan `startPort`, `endPort`, `openPorts` | Requested range and sorted ports observed open on at least one resolved address. The list stores at most 128 ports. |
| Scan `openPortCount`, `truncated` | Total observed open ports, including those beyond the stored list, and whether that list was capped. |
| Scan `probeErrors` | Count of local socket/setup/select/connect errors. Such errors fail the scan. Refusal, timeout, reset, and unreachable outcomes are ordinary ports not observed open. |
| Report `dns`, `ping`, `scan` | Complete component objects with their own `status` and the fields above. |

An `ok` scan can contain no open ports: it means the probes completed, rather than proving reachability. A port omitted from `openPorts` may be refused, filtered, unreachable, or timed out. A successful ping command does not prove zero packet loss; use its metrics. Unsupported localized output can have `status: "ok"` with `null` metrics. Linux/macOS ping runs with `LC_ALL=C`; English Windows summaries are supported, with the precision the OS reports. ICMP can be blocked or restricted independently of TCP.

Schema version 1 replaces earlier `-1` JSON metric sentinels with `null` and replaces report-only counts with full component results. Scripts using the previous output need to adapt. The unimplemented `trace` stub has been removed from the command list.

## Evidence and scope

CTest runs deterministic CLI/output fixtures, English Windows/Linux/macOS ping summary fixtures (including loss, fractions, malformed and localized lines), injected local socket error cases, and native loopback integrations. The integrations cover HTTP-backed IPv4 listeners, closed ports, hostname address fallback, IPv6 when available, 130 open ports, reports, missing/localized ping, and subprocess exit codes. Only IPv6 is conditionally skipped when the runtime lacks loopback support.

CI defines Windows/MSVC, Linux/GCC, and macOS/Clang builds with compiler warnings treated as errors. Local validation during this upgrade used Windows/MinGW; CI results establish the other native platforms. Parser fixtures alone do not prove every OS/runtime behavior.

The tool is deliberately sequential and has bounded result arrays with explicit truncation. It has no traceroute, arbitrary DNS record queries, HTTP probe command, asynchronous scan engine, service fingerprinting, or health score. System ping availability, ICMP privileges, resolver behavior, and firewall policy remain OS responsibilities.
