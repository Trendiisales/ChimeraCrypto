#!/usr/bin/env python3
import re, csv, sys, pathlib

SRC = pathlib.Path("/Users/jo/ChimeraCrypto/src/main.cpp")
OUT = pathlib.Path("/Users/jo/ChimeraCrypto/backtest/engine_roster.csv")

text = SRC.read_text()

cfg_re = re.compile(
    r"chimera::EdgeEngine::Config\s+(\w+)\s*\{(.*?)\};",
    re.DOTALL,
)
field_re = re.compile(r"\.(\w+)\s*=\s*([^,}\n]+?)(?=[,}\n])")
wire_re = re.compile(r"wire_engine\(\s*(\w+)\s*\)")
engine_re = re.compile(r"chimera::EdgeEngine\s+(\w+)\s*\(\s*(\w+)\s*\)")

wired_objs = set(wire_re.findall(text))
obj_to_cfg = {m.group(1): m.group(2) for m in engine_re.finditer(text)}

rows = []
for m in cfg_re.finditer(text):
    cfg_var = m.group(1)
    body = m.group(2)
    f = {}
    for fm in field_re.finditer(body):
        f[fm.group(1)] = fm.group(2).strip().strip('"').rstrip(",")
    if "tag" not in f or "symbol" not in f:
        continue
    rows.append({
        "cfg_var": cfg_var,
        "tag": f.get("tag", ""),
        "symbol": f.get("symbol", ""),
        "kind": f.get("kind", "").replace("chimera::StrategyKind::", ""),
        "tf_secs": f.get("tf_secs", ""),
        "lookback": f.get("lookback", ""),
        "hold_bars": f.get("hold_bars", ""),
        "sl_atr_mult": f.get("sl_atr_mult", ""),
        "atr_period": f.get("atr_period", "14"),
        "bb_k": f.get("bb_k", "2.0"),
        "rsi_threshold": f.get("rsi_threshold", "30.0"),
        "round_trip_bp": f.get("round_trip_bp", "22.0"),
        "max_history": f.get("max_history", "64"),
        "trail_arm_atr": f.get("trail_arm_atr", "1.0"),
        "trail_dist_atr": f.get("trail_dist_atr", "0.4"),
        "trail_tighten_atr": f.get("trail_tighten_atr", "3.0"),
        "trail_tighten_dist_atr": f.get("trail_tighten_dist_atr", "0.25"),
    })

wired_cfg_vars = {obj_to_cfg[o] for o in wired_objs if o in obj_to_cfg}
active = [r for r in rows if r["cfg_var"] in wired_cfg_vars]

with OUT.open("w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(active[0].keys()))
    w.writeheader()
    for r in active:
        w.writerow(r)

print(f"Configs found: {len(rows)}")
print(f"Wired objects: {len(wired_objs)}")
print(f"Active engines (wired + has config): {len(active)}")
print(f"Output: {OUT}")

from collections import Counter
print("\nBy strategy:", Counter(r["kind"] for r in active))
print("By symbol:", dict(sorted(Counter(r["symbol"] for r in active).items())))
