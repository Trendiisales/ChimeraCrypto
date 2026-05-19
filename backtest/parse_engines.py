"""Parse all EdgeEngine::Config blocks from main.cpp into JSON records."""
import re, json, sys
from pathlib import Path

SRC = Path("/Users/jo/ChimeraCrypto/src/main.cpp")
OUT = Path("/Users/jo/ChimeraCrypto/backtest/engines.json")

text = SRC.read_text()

# Match each config block: chimera::EdgeEngine::Config <name>{ ... };
block_re = re.compile(r"chimera::EdgeEngine::Config\s+(\w+_cfg)\s*\{(.*?)\};", re.DOTALL)

def grab(body, field, cast=str):
    m = re.search(rf"\.{field}\s*=\s*([^,\n}}]+)", body)
    if not m: return None
    raw = m.group(1).strip().rstrip(",").strip()
    raw = raw.strip('"')
    if "::" in raw:  # enum like chimera::StrategyKind::RSI_REVERT
        raw = raw.split("::")[-1]
    try:
        if cast is float: return float(raw)
        if cast is int:   return int(raw)
        return raw
    except Exception:
        return raw

records = []
for m in block_re.finditer(text):
    body = m.group(2)
    rec = {
        "var": m.group(1),
        "symbol": grab(body, "symbol"),
        "tag":    grab(body, "tag"),
        "kind":   grab(body, "kind"),
        "tf_secs":     grab(body, "tf_secs", int),
        "lookback":    grab(body, "lookback", int),
        "hold_bars":   grab(body, "hold_bars", int),
        "sl_atr_mult": grab(body, "sl_atr_mult", float),
        "atr_period":  grab(body, "atr_period", int) or 14,
        "bb_k":        grab(body, "bb_k", float) or 2.0,
        "rsi_threshold": grab(body, "rsi_threshold", float) or 30.0,
        "round_trip_bp": grab(body, "round_trip_bp", float) or 20.0,
        "trail_arm_atr":  grab(body, "trail_arm_atr", float) or 1.0,
        "trail_dist_atr": grab(body, "trail_dist_atr", float) or 0.5,
        "trail_tighten_atr":      grab(body, "trail_tighten_atr", float) or 0.0,
        "trail_tighten_dist_atr": grab(body, "trail_tighten_dist_atr", float) or 0.3,
    }
    records.append(rec)

OUT.write_text(json.dumps(records, indent=1))
print(f"parsed {len(records)} engines -> {OUT}")
from collections import Counter
print("by kind:", Counter(r["kind"] for r in records))
print("by symbol:", Counter(r["symbol"] for r in records))
print("by tf:", Counter(r["tf_secs"] for r in records))
