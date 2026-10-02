#!/usr/bin/env python3
import sys
import urllib.request

host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
port = sys.argv[2] if len(sys.argv) > 2 else "8080"

url = f"http://{host}:{port}/api/calc?a=10&b=20"
with urllib.request.urlopen(url, timeout=5) as response:
    body = response.read().decode().strip()
    print(f"HTTP status: {response.status}")
    print(f"10 + 20 = {body}")
