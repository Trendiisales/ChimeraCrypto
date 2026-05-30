#!/usr/bin/env python3
# Export the live S43/S43b engine Configs (designated-initializer C++ structs)
# into the roster CSV the harness --roster mode consumes. Re-gate input.
import re, sys, glob

SRCS = ["../src/engines_s43_repromote.cpp", "../src/engines_s43b_holdout.cpp"]
COLS = ["cfg_var","tag","symbol","kind","tf_secs","lookback","hold_bars","sl_atr_mult",
        "atr_period","bb_k","rsi_threshold","round_trip_bp","max_history","trail_arm_atr",
        "trail_dist_atr","trail_tighten_atr","trail_tighten_dist_atr",
        "ichi_tenkan_period","ichi_kijun_period","ichi_senkou_b_period",
        "keltner_ema_len","keltner_atr_mult"]

# match: chimera::EdgeEngine::Config <var>{ ... };
BLOCK = re.compile(r"EdgeEngine::Config\s+(\w+)\s*\{(.*?)\}\s*;", re.S)

def field(body, name):
    m = re.search(r"\."+name+r"\s*=\s*([^,}\n]+)", body)
    if not m: return ""
    v = m.group(1).strip().strip('"')
    if name == "kind":
        v = v.split("::")[-1]       # chimera::StrategyKind::TSMOM -> TSMOM
    return v

rows = []
for src in SRCS:
    for path in glob.glob(src):
        txt = open(path).read()
        for m in BLOCK.finditer(txt):
            var, body = m.group(1), m.group(2)
            if ".tag=" not in body and ".tag =" not in body: continue
            row = {"cfg_var": var}
            for c in COLS[1:]:
                row[c] = field(body, c)
            rows.append(row)

out = sys.argv[1] if len(sys.argv) > 1 else "roster_308.csv"
with open(out, "w") as f:
    f.write(",".join(COLS)+"\n")
    for r in rows:
        f.write(",".join(str(r.get(c,"")) for c in COLS)+"\n")
print(f"exported {len(rows)} engines -> {out}")
# sanity: kinds + the blew-up engines present?
from collections import Counter
print("kinds:", dict(Counter(r["kind"] for r in rows)))
blew = [r["tag"] for r in rows if r["tag"] in
        ("SEI-TSMOM-H4","RENDER-TSMOM-H2","FET-TSMOM-H12","JUP-TSMOM-H3","NEAR-TSMOM-H12")]
print("blew-up engines present:", blew)
