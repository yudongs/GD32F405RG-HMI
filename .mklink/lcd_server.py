"""LCD framebuffer streaming server for lcd_viewer.html"""
import http.server
import subprocess
import sys
import json
import base64
import threading
import time
import re
import os
import queue

FB_ADDR = "0x200004c0"
FB_SIZE = 4800  # 160 rows * 30 bytes
INTERVAL = 0.5  # polling interval in seconds

clients = []
clients_lock = threading.Lock()
fb_data = bytearray(FB_SIZE)


def read_framebuffer():
    """Read framebuffer from MCU via mklink"""
    try:
        result = subprocess.run(
            [sys.executable, '-m', 'mklink', 'read-ram',
             '--addr', FB_ADDR, '--size', str(FB_SIZE)],
            capture_output=True, text=True, timeout=15,
            encoding='utf-8', errors='replace',
            cwd=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        )
        fb = bytearray(FB_SIZE)
        for line in result.stdout.split('\n'):
            parts = line.strip().split()
            if len(parts) >= 17 and re.match(r'^[0-9A-Fa-f]{8}$', parts[0]):
                addr = int(parts[0], 16)
                offset = addr - int(FB_ADDR, 16)
                if 0 <= offset < FB_SIZE:
                    for i in range(min(16, FB_SIZE - offset)):
                        if i + 1 < len(parts):
                            fb[offset + i] = int(parts[i + 1], 16)
        return bytes(fb)
    except Exception as e:
        print(f"[LCD] read error: {e}")
        return None


def poll_loop():
    """Continuously poll framebuffer and broadcast to clients"""
    global fb_data
    while True:
        data = read_framebuffer()
        if data:
            fb_data = bytearray(data)
            b64 = base64.b64encode(data).decode()
            msg = f"data: {json.dumps({'fb_b64': b64})}\n\n"
            with clients_lock:
                dead = []
                for q in clients:
                    try:
                        q.put_nowait(msg)
                    except queue.Full:
                        dead.append(q)
                for q in dead:
                    clients.remove(q)
        time.sleep(INTERVAL)


class LCDHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        self.directory = os.path.dirname(os.path.abspath(__file__))
        super().__init__(*args, directory=self.directory, **kwargs)

    def do_GET(self):
        if self.path == '/stream':
            self.send_response(200)
            self.send_header('Content-Type', 'text/event-stream')
            self.send_header('Cache-Control', 'no-cache')
            self.send_header('Connection', 'keep-alive')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            q = queue.Queue(maxsize=10)
            with clients_lock:
                clients.append(q)
            try:
                while True:
                    try:
                        msg = q.get(timeout=30)
                        self.wfile.write(msg.encode())
                        self.wfile.flush()
                    except queue.Empty:
                        self.wfile.write(": keepalive\n\n".encode())
                        self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                pass
            finally:
                with clients_lock:
                    if q in clients:
                        clients.remove(q)
        elif self.path == '/' or self.path == '':
            self.path = '/lcd_viewer.html'
            super().do_GET()
        else:
            super().do_GET()

    def log_message(self, format, *args):
        pass  # suppress logs


def main():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    # Start framebuffer polling thread
    poller = threading.Thread(target=poll_loop, daemon=True)
    poller.start()

    # Preload first frame
    time.sleep(0.5)

    # Start HTTP server
    port = 8765
    server = http.server.HTTPServer(('127.0.0.1', port), LCDHandler)
    print(f"[LCD] Server: http://127.0.0.1:{port}")
    print(f"[LCD] Polling framebuffer at {FB_ADDR} every {INTERVAL}s")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[LCD] Stopped")
        server.shutdown()


if __name__ == '__main__':
    main()
