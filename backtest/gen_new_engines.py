#!/usr/bin/env python3
"""Run optimizer on all new engines, capture params, generate C++ and spreadsheet data."""
import subprocess, re, json

# All new engines to test: (symbol, strategy, tf, cost_bp)
NEW_ENGINES = [
    # RSI_REVERT H8
    ("ethusdt","RSI_REVERT","H8",22), ("bnbusdt","RSI_REVERT","H8",22),
    ("dogeusdt","RSI_REVERT","H8",22), ("xrpusdt","RSI_REVERT","H8",22),
    ("aptusdt","RSI_REVERT","H8",22), ("solusdt","RSI_REVERT","H8",22),
    ("linkusdt","RSI_REVERT","H8",22), ("arbusdt","RSI_REVERT","H8",22),
    ("nearusdt","RSI_REVERT","H8",22),
    # BOLLINGER H8
    ("btcusdt","BOLLINGER","H8",22), ("ethusdt","BOLLINGER","H8",22),
    ("solusdt","BOLLINGER","H8",22), ("bnbusdt","BOLLINGER","H8",22),
    ("avaxusdt","BOLLINGER","H8",22), ("linkusdt","BOLLINGER","H8",22),
    ("xrpusdt","BOLLINGER","H8",22), ("dogeusdt","BOLLINGER","H8",22),
    ("suiusdt","BOLLINGER","H8",22), ("aptusdt","BOLLINGER","H8",22),
    ("nearusdt","BOLLINGER","H8",22), ("arbusdt","BOLLINGER","H8",22),
    # RSI_REVERT H16
    ("ethusdt","RSI_REVERT","H16",22), ("bnbusdt","RSI_REVERT","H16",22),
    ("xrpusdt","RSI_REVERT","H16",22), ("linkusdt","RSI_REVERT","H16",22),
    ("nearusdt","RSI_REVERT","H16",22), ("btcusdt","RSI_REVERT","H16",22),
    ("solusdt","RSI_REVERT","H16",22), ("dogeusdt","RSI_REVERT","H16",22),
    # BOLLINGER H16
    ("linkusdt","BOLLINGER","H16",22), ("xrpusdt","BOLLINGER","H16",22),
    ("btcusdt","BOLLINGER","H16",22), ("nearusdt","BOLLINGER","H16",22),
    ("ethusdt","BOLLINGER","H16",22),
    # DONCHIAN H8
    ("xrpusdt","DONCHIAN","H8",22), ("nearusdt","DONCHIAN","H8",22),
    ("suiusdt","DONCHIAN","H8",22), ("btcusdt","DONCHIAN","H8",22),
    ("arbusdt","DONCHIAN","H8",22),
    # DONCHIAN H16
    ("xrpusdt","DONCHIAN","H16",22), ("bnbusdt","DONCHIAN","H16",22),
    ("btcusdt","DONCHIAN","H16",22), ("linkusdt","DONCHIAN","H16",22),
    ("suiusdt","DONCHIAN","H16",22), ("nearusdt","DONCHIAN","H16",22),
    ("solusdt","DONCHIAN","H16",22), ("dogeusdt","DONCHIAN","H16",22),
    # DONCHIAN D2
    ("bnbusdt","DONCHIAN","D2",22), ("xrpusdt","DONCHIAN","D2",22),
    ("btcusdt","DONCHIAN","D2",22), ("ethusdt","DONCHIAN","D2",22),
    ("linkusdt","DONCHIAN","D2",22),
    # DONCHIAN D3
    ("btcusdt","DONCHIAN","D3",22), ("ethusdt","DONCHIAN","D3",22),
    ("xrpusdt","DONCHIAN","D3",22), ("bnbusdt","DONCHIAN","D3",22),
    ("dogeusdt","DONCHIAN","D3",22),
]

TF_SECS = {"H6":21600,"H8":28800,"H12":43200,"H16":57600,"D1":86400,"D2":172800,"D3":259200}

SYM_ENUM = {
    "btcusdt":"BTC","ethusdt":"ETH","solusdt":"SOL","bnbusdt":"BNB",
    "avaxusdt":"AVAX","linkusdt":"LINK","xrpusdt":"XRP","dogeusdt":"DOGE",
    "suiusdt":"SUI","aptusdt":"APT","nearusdt":"NEAR","arbusdt":"ARB"
}

STRAT_KIND = {
    "TSMOM":"chimera::StrategyKind::TSMOM",
    "RSI_REVERT":"chimera::StrategyKind::RSI_REVERT",
    "BOLLINGER":"chimera::StrategyKind::BOLLINGER",
    "DONCHIAN":"chimera::StrategyKind::DONCHIAN"
}

STRAT_SHORT = {"TSMOM":"TSMOM","RSI_REVERT":"RSI","BOLLINGER":"BOLL","DONCHIAN":"DONCH"}

results = []

for sym, strat, tf, cost in NEW_ENGINES:
    cmd = f"./optimizer_general {sym} {strat} {tf} {cost}"
    try:
        out = subprocess.check_output(cmd, shell=True, text=True, timeout=60)
    except:
        print(f"FAILED: {cmd}")
        continue
    
    # Parse recommended config
    lb = hb = sl = ta = td = None
    pf = sharpe = nbr = trades = 0
    
    for line in out.split("\n"):
        if "lookback" in line and "=" in line:
            lb = int(line.split("=")[1].strip())
        elif "hold_bars" in line and "=" in line:
            hb = int(line.split("=")[1].strip())
        elif "sl_atr_mult" in line and "=" in line:
            sl = float(line.split("=")[1].strip())
        elif "trail_arm_atr" in line and "=" in line:
            ta = float(line.split("=")[1].strip())
        elif "trail_dist_atr" in line and "=" in line:
            td = float(line.split("=")[1].strip())
        elif "OOS PF" in line:
            pf = float(line.split("=")[1].strip())
        elif "OOS Sharpe" in line:
            sharpe = float(line.split("=")[1].strip())
        elif "Neighbour %" in line:
            nbr = int(line.split("=")[1].strip().replace("%",""))
        elif "OOS trades" in line:
            trades = int(line.split("=")[1].strip())
    
    if lb is None or trades < 8:
        print(f"SKIP (no result or <8 trades): {sym} {strat} {tf}")
        continue
    
    # Check deploy criteria
    if pf < 1.15 or sharpe < 0.3 or nbr < 40:
        print(f"SKIP (criteria fail): {sym} {strat} {tf} PF={pf} Sharpe={sharpe} Nbr={nbr}")
        continue
    
    tag = f"{SYM_ENUM[sym]}-{STRAT_SHORT[strat]}-{tf}"
    var_base = f"{sym.replace('usdt','') if sym != 'btcusdt' else 'btc'}_{STRAT_SHORT[strat].lower()}_{tf.lower()}"
    
    results.append({
        "sym": sym, "strat": strat, "tf": tf, "cost": cost,
        "tag": tag, "var": var_base,
        "lb": lb, "hb": hb, "sl": sl, "ta": ta, "td": td,
        "pf": pf, "sharpe": sharpe, "nbr": nbr, "trades": trades,
        "tf_secs": TF_SECS[tf], "sym_enum": SYM_ENUM[sym],
        "strat_kind": STRAT_KIND[strat]
    })
    print(f"PASS: {tag} lb={lb} hb={hb} sl={sl} ta={ta} td={td} PF={pf} Sharpe={sharpe} Nbr={nbr}% trades={trades}")

# Save results
with open("new_engines_params.json", "w") as f:
    json.dump(results, f, indent=2)

print(f"\n=== {len(results)} engines passed all criteria ===")
