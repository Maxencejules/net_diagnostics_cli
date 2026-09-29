"""Command contract checks using deterministic C network fixtures."""
import json
import subprocess
import sys
import unittest

EXE = sys.argv.pop(1)


def run(*args):
    return subprocess.run([EXE, *args], capture_output=True, text=True, timeout=5)


class CliContractTests(unittest.TestCase):
    def test_start_port_one_is_not_a_parser_sentinel(self):
        result = run("scan", "localhost", "1", "2", "--json")
        self.assertEqual(result.returncode, 0, result.stderr)
        output = json.loads(result.stdout)
        self.assertEqual((output["startPort"], output["endPort"]), (1, 2))

    def test_invalid_arguments_fail_before_diagnostics(self):
        cases = [
            ("ping", "localhost", "junk", "--json"),
            ("ping", "localhost", "1", "2", "--json"),
            ("ping", "localhost", "0", "--json"),
            ("scan", "localhost", "80junk", "81", "--json"),
            ("dns", "localhost", "--unknown", "--json"),
            ("report", "localhost", "--unknown", "--json"),
            ("ping", "-n", "--json"),
            ("dns", 'host"name', "--json"),
            ("ping", "localhost", "101", "--json"),
            ("scan", "localhost", "65536", "--json"),
            ("scan", "localhost", "2", "1", "--json"),
            ("scan", "localhost", "1", "2", "3", "--json"),
            ("ping", "localhost", "9999999999999999999999999999999", "--json"),
            ("dns", "localhost", "--json", "--json"),
            ("report", "localhost", "1", "2", "--json"),
            ("dns", "fe80::1%eth0", "--json"),
            ("dns", "m\u00fcnich.example", "--json"),
        ]
        for args in cases:
            with self.subTest(args=args):
                result = run(*args)
                self.assertEqual(result.returncode, 2, result.stdout)
                self.assertEqual(result.stdout, "")
                self.assertTrue(result.stderr)

    def test_fractional_rtt_is_preserved(self):
        result = run("ping", "localhost", "1", "--json")
        output = json.loads(result.stdout)
        self.assertEqual(output["rttMs"], {"min": 1.125, "max": 2.25, "avg": 1.75})

    def test_report_stdout_is_one_json_document(self):
        result = run("report", "localhost", "--json")
        self.assertEqual(result.returncode, 0, result.stderr)
        output = json.loads(result.stdout)
        self.assertEqual(output["command"], "report")

    def test_single_scan_port_and_interleaved_json_option(self):
        output = json.loads(run("scan", "localhost", "--json", "443").stdout)
        self.assertEqual((output["startPort"], output["endPort"]), (443, 443))

    def test_report_custom_parameters_and_component_statuses(self):
        output = json.loads(run("report", "localhost", "1", "80", "81", "--json").stdout)
        self.assertEqual(output["schemaVersion"], 1)
        self.assertEqual(output["ping"]["count"], 1)
        self.assertEqual((output["scan"]["startPort"], output["scan"]["endPort"]), (80, 81))
        self.assertTrue(all(output[name]["status"] == "ok" for name in ("dns", "ping", "scan")))

    def test_operational_failure_is_json_with_failed_components(self):
        result = run("report", "failure.test", "--json")
        self.assertEqual(result.returncode, 1)
        output = json.loads(result.stdout)
        self.assertEqual(output["status"], "failed")
        for name in ("dns", "ping", "scan"):
            self.assertEqual(output[name]["status"], "failed")
        self.assertIsNone(output["ping"]["lossPercent"])

    def test_success_with_unknown_metrics_does_not_invent_measurements(self):
        output = json.loads(run("ping", "unknown.test", "--json").stdout)
        self.assertEqual(output["status"], "ok")
        self.assertIsNone(output["lossPercent"])
        self.assertEqual(output["rttMs"], {"min": None, "max": None, "avg": None})


if __name__ == "__main__":
    unittest.main()
