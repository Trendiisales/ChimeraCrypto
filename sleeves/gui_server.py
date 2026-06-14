#!/usr/bin/env python3
"""Serve the original Chimera GUI (index.html, all blocks intact) for the paper
sleeves. Provides the same endpoints the old dashboard polls:
  /api/state2  -> {spot_prices, engines, build, startup_ts}  (live ticker prices)
  /api/trades  -> paper trade ledger (data/trades.json)
  /api/kill    -> no-op (paper, no real positions)
Read-only / paper. Bind + exposure decided by the operator (see README)."""
import http.server, socketserver, os, json, time, urllib.request
DIR = os.path.dirname(os.path.abspath(__file__))
PORT = int(os.environ.get("SLEEVE_GUI_PORT","9444"))
# ticker symbols shown on the top bar (match the old GUI SYMBOLS list)
TICKER = ["BTC","ETH","SOL","BNB","AVAX","LINK","XRP","DOGE","NEAR","SUI","APT",
          "ARB","PEPE","WIF","FET","ONDO","TIA","HBAR","INJ","ADA","TRX","SEI"]
_spot_cache = {"ts":0,"px":{}}

def live_spot():
    """Live Binance prices for the ticker (cached 15s). Falls back to local closes."""
    if time.time()-_spot_cache["ts"] < 15 and _spot_cache["px"]:
        return _spot_cache["px"]
    px={}
    try:
        raw=json.loads(urllib.request.urlopen("https://api.binance.com/api/v3/ticker/price",timeout=8).read())
        m={r["symbol"]:float(r["price"]) for r in raw}
        for s in TICKER:
            if s+"USDT" in m: px[s.lower()+"usdt"]=m[s+"USDT"]
    except Exception: pass
    if px: _spot_cache.update(ts=time.time(),px=px)
    return px

def state2():
    p=os.path.join(DIR,"api_state.json")
    st=json.load(open(p)) if os.path.exists(p) else {"engines":[],"build":"sleeves","startup_ts":0,"spot_local":{}}
    spot=dict(st.get("spot_local",{})); spot.update(live_spot())   # local closes + live ticker overlay
    return {"spot_prices":spot,"engines":st.get("engines",[]),
            "build":st.get("build","sleeves"),"startup_ts":st.get("startup_ts",0)}

class H(http.server.SimpleHTTPRequestHandler):
    def __init__(self,*a,**k): super().__init__(*a,directory=DIR,**k)
    def _json(self,obj,code=200):
        b=json.dumps(obj).encode(); self.send_response(code)
        self.send_header("Content-Type","application/json")
        self.send_header("Cache-Control","no-store"); self.end_headers(); self.wfile.write(b)
    def do_POST(self):
        if self.path.startswith("/api/kill"): return self._json({"ok":True,"note":"paper — no real positions"})
        self.send_response(404); self.end_headers()
    def do_GET(self):
        if self.path=="/": self.path="/index.html"; return super().do_GET()
        if self.path.startswith("/api/state2"): return self._json(state2())
        if self.path.startswith("/api/trades"):
            tp=os.path.join(DIR,"data","trades.json")
            return self._json(json.load(open(tp)) if os.path.exists(tp) else [])
        return super().do_GET()
    def log_message(self,*a): pass

if __name__=="__main__":
    socketserver.TCPServer.allow_reuse_address=True
    bind=os.environ.get("SLEEVE_GUI_BIND","127.0.0.1")
    with socketserver.TCPServer((bind,PORT),H) as s:
        print(f"Chimera Sleeves GUI (old layout) on http://{bind}:{PORT}")
        s.serve_forever()
