"""Native CLI integration tests. Servers and diagnostic traffic stay on loopback."""
import faulthandler

if __name__ == "__main__":
    faulthandler.dump_traceback_later(20, repeat=True)

import contextlib
import http.server
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import threading
import unittest
import urllib.request

EXE = str(Path(sys.argv.pop(1)).resolve())
FIXTURES = Path(__file__).parent / "fixtures"


def run(*args, env=None):
    result = subprocess.run([EXE, *args], capture_output=True, text=True, env=env, timeout=15)
    output = json.loads(result.stdout)
    if output["schemaVersion"] != 1:
        raise AssertionError(output)
    return result, output


class HealthHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"status":"ok"}')

    def log_message(self, *args):
        pass


@contextlib.contextmanager
def endpoint(family=socket.AF_INET):
    class Server(http.server.ThreadingHTTPServer):
        address_family = family

    address = "127.0.0.1" if family == socket.AF_INET else "::1"
    server = Server((address, 0), HealthHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield address, server.server_port
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


@contextlib.contextmanager
def fake_ping(text, code):
    with tempfile.TemporaryDirectory() as directory:
        directory = Path(directory)
        (directory / "output.txt").write_text(text, encoding="utf-8")
        if os.name == "nt":
            program = directory / "ping.cmd"
            program.write_text(f'@echo off\ntype "{directory / "output.txt"}"\nexit /b {code}\n')
        else:
            program = directory / "ping"
            program.write_text(f'#!/bin/sh\ncat "{directory / "output.txt"}"\nexit {code}\n')
            program.chmod(0o700)
        env = os.environ.copy()
        env["PATH"] = str(directory) + os.pathsep + env.get("PATH", "")
        yield env


class EndpointTests(unittest.TestCase):
    def test_ipv4_http_listener_open_and_reserved_nonlistener_not_open(self):
        with endpoint() as (host, port), socket.socket() as closed:
            # A bound, non-listening socket reserves a stable port without accepting connections.
            closed.bind(("127.0.0.1", 0))
            closed_port = closed.getsockname()[1]
            with urllib.request.urlopen(f"http://{host}:{port}/health", timeout=3) as response:
                self.assertEqual(json.load(response), {"status": "ok"})
            result, output = run("scan", host, str(port), "--json")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output["openPorts"], [port])
            self.assertEqual(output["openPortCount"], 1)
            self.assertFalse(output["truncated"])
            self.assertEqual(output["probeErrors"], 0)
            result, output = run("scan", host, str(closed_port), "--json")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output["openPorts"], [])

    def test_localhost_checks_other_resolved_addresses(self):
        with endpoint() as (_, port):
            result, output = run("scan", "localhost", str(port), "--json")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output["openPorts"], [port])

    def test_ipv6_loopback_if_available(self):
        if not socket.has_ipv6:
            self.skipTest("Python/runtime has no IPv6 support")
        try:
            server = endpoint(socket.AF_INET6)
            address, port = server.__enter__()
        except OSError as error:
            self.skipTest(f"IPv6 loopback unavailable: {error}")
        try:
            result, output = run("scan", address, str(port), "--json")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output["openPorts"], [port])
        finally:
            server.__exit__(None, None, None)

    def test_dns_loopback_addresses_are_unique(self):
        result, output = run("dns", "localhost", "--json")
        self.assertEqual(result.returncode, 0, result.stderr)
        records = {(item["family"], item["ip"]) for item in output["records"]}
        self.assertEqual(len(records), len(output["records"]))
        self.assertIn(("IPv4", "127.0.0.1"), records)

    def test_report_preserves_component_results(self):
        with endpoint() as (host, port):
            result, output = run("report", host, "1", str(port), str(port), "--json")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output["status"], "ok")
            self.assertTrue(output["dns"]["records"])
            self.assertEqual(output["ping"]["count"], 1)
            self.assertEqual(output["ping"]["systemExitCode"], 0)
            self.assertEqual(output["scan"]["openPorts"], [port])

    def test_capacity_is_explicit_after_128_open_ports(self):
        listeners = []
        # Reserve a contiguous range before scanning; retry collisions with other local processes.
        for base in range(32000, 50000, 131):
            try:
                for port in range(base, base + 130):
                    listener = socket.socket()
                    listeners.append(listener)
                    listener.bind(("127.0.0.1", port))
                    listener.listen()
                break
            except OSError:
                for listener in listeners:
                    listener.close()
                listeners.clear()
        else:
            self.fail("Could not reserve 130 loopback ports for capacity regression")
        try:
            result, output = run("scan", "127.0.0.1", str(base), str(base + 129), "--json")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output["openPortCount"], 130)
            self.assertEqual(output["openPorts"], list(range(base, base + 128)))
            self.assertTrue(output["truncated"])
        finally:
            for listener in listeners:
                listener.close()

    def test_ping_exit_code_is_normalized_and_parsed_metrics_survive_failure(self):
        with fake_ping((FIXTURES / "linux.txt").read_text(), 7) as env:
            result, output = run("ping", "127.0.0.1", "1", "--json", env=env)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(output["status"], "failed")
        self.assertEqual(output["systemExitCode"], 7)
        self.assertAlmostEqual(output["rttMs"]["avg"], 0.042)

    def test_localized_metrics_remain_unknown_even_if_ping_succeeded(self):
        with fake_ping((FIXTURES / "localized.txt").read_text(), 0) as env:
            result, output = run("ping", "127.0.0.1", "1", "--json", env=env)
        self.assertEqual(result.returncode, 0)
        self.assertIsNone(output["lossPercent"])
        self.assertEqual(output["rttMs"], {"min": None, "max": None, "avg": None})

    def test_missing_ping_keeps_json_failure_parseable(self):
        with tempfile.TemporaryDirectory() as directory:
            env = os.environ.copy()
            env["PATH"] = directory
            result, output = run("ping", "127.0.0.1", "1", "--json", env=env)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(output["status"], "failed")
        self.assertNotEqual(output["systemExitCode"], 0)
        self.assertIsNone(output["lossPercent"])
        self.assertEqual(output["rttMs"], {"min": None, "max": None, "avg": None})


if __name__ == "__main__":
    try:
        unittest.main(verbosity=2)
    finally:
        faulthandler.cancel_dump_traceback_later()
