"""Start a local HTTP endpoint and run a bounded diagnostic report against it."""
import argparse
import http.server
import json
from pathlib import Path
import socketserver
import subprocess
import threading
import urllib.request

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--exe", type=Path, help="Built netdiag executable")
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
exe = args.exe
if exe is None:
    exe = next((path for path in (root / "build/netdiag", root / "build/netdiag.exe",
                                root / "build/Release/netdiag.exe") if path.is_file()), None)
if exe is None:
    parser.error("Build netdiag first, or provide --exe.")


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"status":"ok"}')

    def log_message(self, *args):
        pass


class LoopbackHTTPServer(http.server.ThreadingHTTPServer):
    def server_bind(self):
        # The numeric demo endpoint does not need HTTPServer's reverse DNS lookup.
        socketserver.TCPServer.server_bind(self)
        self.server_name, self.server_port = self.server_address[:2]


server = LoopbackHTTPServer(("127.0.0.1", 0), Handler)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    url = f"http://127.0.0.1:{server.server_port}/health"
    with urllib.request.urlopen(url, timeout=3) as response:
        http_status = response.status
        http_health = json.load(response)
    result = subprocess.run([str(exe.resolve()), "report", "127.0.0.1", "1",
                             str(server.server_port), str(server.server_port), "--json"],
                            capture_output=True, text=True, timeout=20)
    print(json.dumps({"demoEndpoint": url, "httpStatus": http_status,
                      "httpHealth": http_health, "report": json.loads(result.stdout)}, indent=2))
finally:
    server.shutdown()
    server.server_close()
    thread.join(timeout=5)
raise SystemExit(result.returncode)
