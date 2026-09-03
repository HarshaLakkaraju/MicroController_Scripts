#!/usr/bin/env python3
"""
Mock test for Mochi Ambient Service.
Runs a local server mimicking Mochi to test status queries and animation notifications.
"""
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading
import urllib.request
import urllib.parse
import json
import random
import time

PORT = 8899

class MockMochiHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass  # Suppress default server log

    def do_GET(self):
        if self.path == "/status":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"mode": "SITTING"}')
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        if self.path == "/notify":
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length).decode()
            print(f"[Mock Server] Received notification trigger: {body}")
            self.send_response(200)
            self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

def run_test():
    server = HTTPServer(("127.0.0.1", PORT), MockMochiHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    print(f"🍡 Mock Mochi server running on http://127.0.0.1:{PORT}")

    ANIMATIONS = ["fly", "suspicious", "dizzy", "shy", "thinking"]
    MOCHI_URL = f"http://127.0.0.1:{PORT}"

    print("🍡 Querying /status...")
    response = urllib.request.urlopen(f"{MOCHI_URL}/status", timeout=2)
    data = json.loads(response.read().decode())
    print(f"✅ Mochi status response: {data}")

    if data.get("mode") == "SITTING":
        chosen_anim = random.choice(ANIMATIONS)
        print(f"🍡 Triggering animation: {chosen_anim}")
        post_data = urllib.parse.urlencode({"type": chosen_anim}).encode()
        urllib.request.urlopen(f"{MOCHI_URL}/notify", data=post_data, timeout=2)
        print("✅ Successfully sent animation trigger!")

    server.shutdown()
    print("🎉 All mock tests passed successfully!")

if __name__ == "__main__":
    run_test()
