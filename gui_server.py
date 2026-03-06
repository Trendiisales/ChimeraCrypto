#!/usr/bin/env python3
import re
import time
import json
import threading
from pathlib import Path
from http.server import HTTPServer, SimpleHTTPRequestHandler
from datetime import datetime

class EngineState:
    def __init__(self):
        self.equity = 10000.0
        self.day_pnl = 0.0
        self.pnl = 0.0
        self.unrealized_pnl = 0.0
        self.trades_today = 0
        self.positions = 0
        self.win_rate = 0.0
        self.sharpe_ratio = 0.0
        self.latency_ms = 0.0
        self.orders_sent = 0
        self.fills_received = 0
        self.btc_position = 0.0
        self.eth_position = 0.0
        self.sol_position = 0.0
        self.governor = "ACTIVE"
        self.exposure_usd = 0.0

state = EngineState()
state_lock = threading.Lock()

def parse_logs():
    global state
    try:
        log_dir = Path("/home/jo/ChimeraCrypto/logs")
        log_files = list(log_dir.glob("chimera*.log"))
        if not log_files:
            return
        
        latest_log = max(log_files, key=lambda p: p.stat().st_mtime)
        with open(latest_log, 'r') as f:
            lines = f.readlines()[-5000:]
        
        total_pnl = 0.0
        trades = 0
        wins = 0
        
        for line in lines:
            # Parse EXIT for PnL
            exit_match = re.search(r'\[EXIT\].*total_pnl=([-\d.]+)', line)
            if exit_match:
                total_pnl = float(exit_match.group(1))
                trades += 1
                pnl_match = re.search(r'pnl=([-\d.]+)bp', line)
                if pnl_match and float(pnl_match.group(1)) > 0:
                    wins += 1
            
            # Parse latency
            lat_match = re.search(r'lat_p95=([\d.]+)ms', line)
            if lat_match:
                state.latency_ms = float(lat_match.group(1))
        
        with state_lock:
            state.day_pnl = total_pnl
            state.pnl = total_pnl
            state.equity = 10000.0 + total_pnl
            state.trades_today = trades
            state.win_rate = wins / trades if trades > 0 else 0.0
            state.orders_sent = trades
            state.fills_received = trades
            
    except Exception as e:
        print(f"[GUI] Parse error: {e}")

def log_watcher():
    while True:
        parse_logs()
        time.sleep(1)

class GUIHandler(SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        pass
    
    def do_GET(self):
        if self.path == '/':
            # Serve old GUI HTML
            with open('/home/jo/ChimeraCrypto/gui/index.html', 'rb') as f:
                content = f.read()
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.end_headers()
            self.wfile.write(content)
        elif self.path.endswith('.js'):
            try:
                with open('/home/jo/ChimeraCrypto/gui' + self.path, 'rb') as f:
                    content = f.read()
                self.send_response(200)
                self.send_header('Content-Type', 'application/javascript')
                self.end_headers()
                self.wfile.write(content)
            except:
                self.send_error(404)
        elif self.path.endswith('.css'):
            try:
                with open('/home/jo/ChimeraCrypto/gui' + self.path, 'rb') as f:
                    content = f.read()
                self.send_response(200)
                self.send_header('Content-Type', 'text/css')
                self.end_headers()
                self.wfile.write(content)
            except:
                self.send_error(404)
        else:
            self.send_error(404)

class FakeWebSocket:
    def __init__(self):
        self.clients = []
    
    def send_state(self):
        while True:
            with state_lock:
                data = {
                    'equity': state.equity,
                    'day_pnl': state.day_pnl,
                    'pnl': state.pnl,
                    'unrealized_pnl': state.unrealized_pnl,
                    'trades_today': state.trades_today,
                    'positions': state.positions,
                    'win_rate': state.win_rate,
                    'sharpe_ratio': state.sharpe_ratio,
                    'latency_ms': state.latency_ms,
                    'orders_sent': state.orders_sent,
                    'fills_received': state.fills_received,
                    'btc_position': state.btc_position,
                    'eth_position': state.eth_position,
                    'sol_position': state.sol_position,
                    'governor': state.governor,
                    'exposure_usd': state.exposure_usd
                }
            time.sleep(1)

watcher = threading.Thread(target=log_watcher, daemon=True)
watcher.start()

server = HTTPServer(('0.0.0.0', 8080), GUIHandler)
print("[GUI] Serving old GUI on port 8080 with log-based data")
server.serve_forever()
