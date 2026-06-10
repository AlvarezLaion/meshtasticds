#!/usr/bin/env bash
# Mock bridge server for emulator testing
# Listens on TCP port 4444, echoes messages back as {"text":"Echo: <original>"}

PORT=4444

# Use Python for more robust TCP handling
python3 << 'PYTHON_EOF'
import socket
import json
import sys

PORT = 4444
HOST = '127.0.0.1'

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(1)

print(f"Mock bridge listening on {HOST}:{PORT} ...")
sys.stdout.flush()

try:
    while True:
        client, addr = server.accept()
        print(f"Client connected from {addr}", file=sys.stderr)
        sys.stderr.flush()

        try:
            # Read incoming message
            data = client.recv(1024).decode('utf-8', errors='ignore')
            if not data:
                continue

            line = data.strip()
            print(f"Bridge received: {line}", file=sys.stderr)
            sys.stderr.flush()

            # Parse JSON and echo back
            try:
                msg = json.loads(line)
                text = msg.get('text', '')
                if text:
                    response = json.dumps({"text": f"Echo: {text}"})
                    client.send((response + '\n').encode('utf-8'))
            except json.JSONDecodeError:
                print(f"JSON parse error: {line}", file=sys.stderr)
                sys.stderr.flush()
        finally:
            client.close()
except KeyboardInterrupt:
    print("\nShutting down...", file=sys.stderr)
finally:
    server.close()
PYTHON_EOF
