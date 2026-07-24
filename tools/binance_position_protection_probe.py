#!/usr/bin/env python3
# UNPROTECTED-POSITION check for the Binance spot book (crypto twin of Omega's
# tools/position_protection_probe.py). Every held spot position MUST have a
# RESTING protective SELL order at Binance (a STOP_LOSS_LIMIT / STOP_LOSS stop,
# or any resting SELL that reduces it). Any held asset without one = RED — that
# is a NAKED position that will NOT be protected if the bot dies. This is the
# crypto edition of the check that was missing when BMY sat 36h unprotected.
#
# READ-ONLY. Only GET endpoints are signed/called (account, openOrders) plus the
# public ticker/exchangeInfo. It NEVER POSTs/DELETEs an order (audit-read-only-
# never-mutate rule). Placing/healing stops is the ENGINE's job, not the monitor's.
#
# Exit 0 = all held positions protected (or nothing held / shadow / unreachable —
#          never false-alarm on a query failure, mirror of the Omega probe).
# Exit 2 = one or more NAKED positions -> the watch script alerts.
#
# Pure stdlib (urllib+hmac+hashlib) so it runs headless on the box with no deps.
# Run on the box:  python3 binance_position_protection_probe.py [creds.json]
import sys, os, json, time, hmac, hashlib, urllib.parse, urllib.request

BASE = "https://api.binance.com"
# Assets that are cash, not a position to protect.
STABLE = {"USDT", "BUSD", "USDC", "FDUSD", "TUSD", "DAI"}
MIN_NOTIONAL_USD = 5.0   # Binance spot floor — dust below this is not a position.

def _load_creds(path):
    with open(path) as f:
        d = json.load(f)
    key = d.get("api_key", "")
    sec = d.get("api_secret") or d.get("secret_key") or ""
    shadow = str(d.get("shadow_mode", True)).lower() != "false"
    return key, sec, shadow

def _get(path, key, params=None):
    url = BASE + path
    if params is not None:
        url += "?" + params
    req = urllib.request.Request(url, headers={"X-MBX-APIKEY": key} if key else {})
    with urllib.request.urlopen(req, timeout=12) as r:
        return json.load(r)

def _signed_get(path, key, sec, extra=""):
    qs = "recvWindow=5000&timestamp=" + str(int(time.time() * 1000))
    if extra:
        qs = extra + "&" + qs
    sig = hmac.new(sec.encode(), qs.encode(), hashlib.sha256).hexdigest()
    return _get(path, key, qs + "&signature=" + sig)

def main():
    creds = sys.argv[1] if len(sys.argv) > 1 else \
        os.path.expanduser("~/ChimeraCrypto/config/binance_credentials.json")
    try:
        key, sec, shadow = _load_creds(creds)
    except Exception as e:
        print(f"PROBE-UNREACHABLE: credentials {creds}: {e}"); sys.exit(0)
    if shadow:
        print("SHADOW mode — no real Binance positions to protect"); sys.exit(0)
    if not key or not sec:
        print("PROBE-UNREACHABLE: empty credentials"); sys.exit(0)

    try:
        acct   = _signed_get("/api/v3/account", key, sec)
        opens  = _signed_get("/api/v3/openOrders", key, sec)          # ALL symbols
        prices = {p["symbol"]: float(p["price"]) for p in _get("/api/v3/ticker/price", key)}
    except Exception as e:
        print(f"PROBE-UNREACHABLE: {e}"); sys.exit(0)   # can't verify -> don't false-alarm

    # Held positions: non-stable base assets whose total value clears MIN_NOTIONAL.
    held = {}
    for b in acct.get("balances", []):
        asset = b.get("asset", "")
        qty = float(b.get("free", 0)) + float(b.get("locked", 0))
        if asset in STABLE or qty <= 0:
            continue
        px = prices.get(asset + "USDT", 0.0)
        if px > 0 and qty * px >= MIN_NOTIONAL_USD:
            held[asset] = (qty, qty * px)

    # A symbol is PROTECTED if it has a resting SELL order (a stop, or any resting
    # SELL that reduces the long). Mirror of the Omega probe's protected-set logic.
    protected = set()
    for o in opens:
        if o.get("side") == "SELL" and o.get("status") in ("NEW", "PARTIALLY_FILLED"):
            protected.add(o.get("symbol", ""))

    naked = [(a, q, v) for a, (q, v) in held.items()
             if (a + "USDT") not in protected]

    if naked:
        print("UNPROTECTED POSITIONS (no resting Binance stop/close): " +
              ", ".join(f"{a}({q:.6f}=${v:.0f})" for a, q, v in naked))
        sys.exit(2)
    if held:
        print("OK: every held Binance position has a resting protective SELL "
              f"({', '.join(sorted(held))})")
    else:
        print("OK: no held Binance positions")
    sys.exit(0)

if __name__ == "__main__":
    main()
