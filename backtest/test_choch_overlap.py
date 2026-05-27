"""
test_choch_overlap.py — Test if ChoCH catches different bars than TSMOM/DONCH.

For each symbol × TF:
  - Generate ChoCH signal bars
  - Generate TSMOM signal bars (lookback=20)
  - Generate DONCHIAN signal bars (lookback=20)
  - Measure: % of ChoCH bars also in TSMOM ∪ DONCH (overlap rate)
  - Measure: % of ChoCH bars NOT in TSMOM ∪ DONCH (novel rate)

Low overlap = ChoCH catches different setups = valuable.
High overlap = ChoCH redundant.
"""
import sys
from pathlib import Path
import numpy as np
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import load_h1, resample, TF_MAP
from test_new_signals import sig_choch, sig_rsi_divergence
from validate_with_protections import sig_tsmom, sig_donchian

SYMBOLS = ["btcusdt","ethusdt","bnbusdt","solusdt","xrpusdt","dogeusdt",
           "linkusdt","avaxusdt","aptusdt","arbusdt","nearusdt","suiusdt"]
TFS = ["4h","6h","8h","12h","1D"]

def main():
    rows = []
    for sym in SYMBOLS:
        h1 = load_h1(sym)
        if h1 is None: continue
        for tf in TFS:
            df = resample(h1, tf)
            if len(df) < 150: continue

            choch = sig_choch(df, pivot_len=5)
            rsidiv = sig_rsi_divergence(df, rsi_period=14, pivot_len=5)
            tsmom = sig_tsmom(df, 20)
            donch = sig_donchian(df, 20)

            # Bars where ANY existing signal fires
            existing = tsmom | donch
            # Within +/- 2 bars of an existing signal (catch near-matches)
            existing_neighborhood = np.zeros(len(df), dtype=bool)
            for shift in range(-2, 3):
                shifted = np.roll(existing, shift)
                # zero out wrapped indices
                if shift > 0:
                    shifted[:shift] = False
                elif shift < 0:
                    shifted[shift:] = False
                existing_neighborhood |= shifted

            n_choch = int(choch.sum())
            n_rsidiv = int(rsidiv.sum())
            if n_choch:
                choch_overlap = int((choch & existing_neighborhood).sum())
                choch_novel = n_choch - choch_overlap
            else:
                choch_overlap = choch_novel = 0
            if n_rsidiv:
                rsi_overlap = int((rsidiv & existing_neighborhood).sum())
                rsi_novel = n_rsidiv - rsi_overlap
            else:
                rsi_overlap = rsi_novel = 0

            rows.append({
                "sym": sym.upper().replace("USDT",""), "tf": tf,
                "n_choch": n_choch,
                "choch_overlap_pct": (choch_overlap / max(1, n_choch)) * 100,
                "choch_novel_pct": (choch_novel / max(1, n_choch)) * 100,
                "n_rsidiv": n_rsidiv,
                "rsidiv_overlap_pct": (rsi_overlap / max(1, n_rsidiv)) * 100,
                "rsidiv_novel_pct": (rsi_novel / max(1, n_rsidiv)) * 100,
            })

    df = pd.DataFrame(rows)
    print("=== Signal overlap with existing TSMOM/DONCHIAN (±2 bar window) ===")
    print()
    print(df.to_string(index=False, formatters={
        "choch_overlap_pct":"{:.1f}%".format, "choch_novel_pct":"{:.1f}%".format,
        "rsidiv_overlap_pct":"{:.1f}%".format, "rsidiv_novel_pct":"{:.1f}%".format,
    }))
    print()
    print("=== AGGREGATE ===")
    total_choch = df.n_choch.sum()
    total_choch_novel = (df.n_choch * df.choch_novel_pct / 100).sum()
    total_rsidiv = df.n_rsidiv.sum()
    total_rsidiv_novel = (df.n_rsidiv * df.rsidiv_novel_pct / 100).sum()
    print(f"ChoCH: {total_choch:.0f} signals, {total_choch_novel:.0f} novel ({100*total_choch_novel/total_choch:.1f}%)")
    print(f"RSI div: {total_rsidiv:.0f} signals, {total_rsidiv_novel:.0f} novel ({100*total_rsidiv_novel/total_rsidiv:.1f}%)")

if __name__ == "__main__":
    main()
