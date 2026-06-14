#!/usr/bin/env python3
"""Serve the Chimera Sleeves dashboard on :9444. Static files + /refresh that
regenerates gui_data.json on demand. Read-only, shadow — no order endpoints."""
import http.server, socketserver, os, subprocess, sys
DIR = os.path.dirname(os.path.abspath(__file__))
PORT = int(os.environ.get("SLEEVE_GUI_PORT", "9444"))

class H(http.server.SimpleHTTPRequestHandler):
    def __init__(self,*a,**k): super().__init__(*a,directory=DIR,**k)
    def do_GET(self):
        if self.path=="/" : self.path="/sleeves_gui.html"
        elif self.path.startswith("/refresh"):
            try: subprocess.run([sys.executable,"gui_data.py"],cwd=DIR,timeout=120,check=True)
            except Exception as e:
                self.send_response(500); self.end_headers(); self.wfile.write(str(e).encode()); return
            self.send_response(200); self.send_header("Content-Type","text/plain"); self.end_headers()
            self.wfile.write(b"refreshed"); return
        return super().do_GET()
    def log_message(self,*a): pass

if __name__=="__main__":
    # refresh once on boot so the page has data
    try: subprocess.run([sys.executable,"gui_data.py"],cwd=DIR,timeout=120)
    except Exception as e: print("warn: initial gui_data gen:",e)
    socketserver.TCPServer.allow_reuse_address=True
    with socketserver.TCPServer(("0.0.0.0",PORT),H) as s:
        print(f"Chimera Sleeves GUI on http://0.0.0.0:{PORT}")
        s.serve_forever()
