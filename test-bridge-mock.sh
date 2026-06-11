#!/usr/bin/env bash
# Mock bridge server for emulator testing.
# Mirrors BridgeModule behavior: keeps the TCP connection open, sends a
# greeting on connect, and echoes each JSON line back as
# {"text":"Echo: <original>"}\n until the client disconnects.

python3 << 'PYTHON_EOF'
import socket
import json
import sys

HOST = '127.0.0.1'
PORT = 4444

def log(msg):
    print(msg, file=sys.stderr, flush=True)

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(1)

print(f"Mock bridge listening on {HOST}:{PORT} ...", flush=True)

try:
    while True:
        client, addr = server.accept()
        log(f"Client connected from {addr}")
        try:
            client.sendall(b'{"text":"Mock bridge online"}\n')
            buf = b''
            while True:
                data = client.recv(1024)
                if not data:
                    log("Client disconnected")
                    break
                buf += data
                while b'\n' in buf:
                    line, buf = buf.split(b'\n', 1)
                    line = line.strip()
                    if not line:
                        continue
                    log(f"Bridge received: {line.decode('utf-8', 'replace')}")
                    try:
                        msg = json.loads(line)
                    except json.JSONDecodeError:
                        log(f"JSON parse error: {line!r}")
                        continue
                    text = msg.get('text', '')
                    if text:
                        response = json.dumps({"text": f"Echo: {text}"})
                        client.sendall((response + '\n').encode('utf-8'))
        except (ConnectionResetError, BrokenPipeError):
            log("Client connection reset")
        finally:
            client.close()
except KeyboardInterrupt:
    log("\nShutting down...")
finally:
    server.close()
PYTHON_EOF
