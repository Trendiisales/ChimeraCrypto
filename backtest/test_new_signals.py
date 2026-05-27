"""
test_new_signals.py — Validate ChoCH + RSI divergence + combined as new signal kinds.

ChoCH (Change of Character):
  Pivot-based structural break.
  Track swing highs/lows using N-bar pivot (price = max/min of N bars each side).
  Bullish ChoCH = close > last lower-high (after a downtrend).
  Bearish ChoCH = close < last higher-low (after an uptrend). [SKIPPED — long-only]

  We need market regime context: only fire bullish ChoCH when prior structure
  was downtrend (last swing low < prior swing low).

RSI divergence (bullish):
  Price makes new low (close <= prior swing low) but RSI value at this point
  is HIGHER than RSI at prior swing low. Indicates weakening downside momentum.

Combined:
  Both signals fire within a window of N bars.

Backtests on all 12 symbols × multiple TFs. Uses same full protection stack.
"""
import json, sys, math
from pathlib import Path
import numpy as np
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import (
    load_h1, resample, atr, rsi, metrics, passes, TF_MAP, backtest_engine
)

ROOT = Path("/Users/jo/ChimeraCrypto/backtest")
OUT  = ROOT / "test_new_signals_results.csv"

SYMBOLS = ["btcusdt","ethusdt","bnbusdt","solusdt","xrpusdt","dogeusdt",
           "linkusdt","avaxusdt","aptusdt","arbusdt","nearusdt","suiusdt"]
TFS = ["2h","3h","4h","6h","8h","12h","1D","2D"]
TF_TO_SECS = {v:k for k,v in TF_MAP.items()}

# ── Swing pivot detection ────────────────────────────────────────────────────
def find_pivots(values, n_left=5, n_right=5, is_high=True):
    """Mark index i as pivot if values[i] is max (or min) over [i-n_left, i+n_right]."""
    arr = np.asarray(values)
    sz = len(arr)
    out = np.zeros(sz, dtype=bool)
    for i in range(n_left, sz - n_right):
        window = arr[i - n_left : i + n_right + 1]
        center = arr[i]
        if is_high:
            if center >= np.max(window) and np.sum(window == center) == 1:
                out[i] = True
        else:
            if center <= np.min(window) and np.sum(window == center) == 1:
                out[i] = True
    return out

# ── ChoCH signal ─────────────────────────────────────────────────────────────
def sig_choch(df, pivot_len=5):
    """Bullish ChoCH = close > last_lower_high after the structure was bearish.

    Structure tracking: walk pivots in order, maintain (last_swing_high, last_swing_low).
    When new swing_low < prior swing_low -> downtrend confirmed. Then next time
    close[i] > most recent swing_high (which IS a lower-high in the downtrend),
    that's ChoCH bullish.
    """
    h = df.high.values
    l = df.low.values
    c = df.close.values
    sz = len(df)
    pivot_high = find_pivots(h, pivot_len, pivot_len, is_high=True)
    pivot_low  = find_pivots(l, pivot_len, pivot_len, is_high=False)
    out = np.zeros(sz, dtype=bool)
    last_swing_high = None  # (idx, price)
    last_swing_low  = None
    prev_swing_high = None
    prev_swing_low  = None
    downtrend = False  # last swing low broke prior low
    target_high_to_break = None  # price level for ChoCH bullish trigger
    for i in range(sz):
        # Update structure when a confirmed pivot is observed
        # Pivots are confirmed pivot_len bars later (need right-side data)
        # So check pivot at i - pivot_len (confirmed pivot)
        conf_i = i - pivot_len
        if conf_i >= 0:
            if pivot_high[conf_i]:
                prev_swing_high = last_swing_high
                last_swing_high = (conf_i, h[conf_i])
                # If we're in downtrend, this new swing high becomes our break target
                if downtrend:
                    target_high_to_break = h[conf_i]
            if pivot_low[conf_i]:
                prev_swing_low = last_swing_low
                last_swing_low = (conf_i, l[conf_i])
                if prev_swing_low is not None and last_swing_low[1] < prev_swing_low[1]:
                    downtrend = True
                    # target is the most recent swing high
                    if last_swing_high is not None:
                        target_high_to_break = last_swing_high[1]
                elif prev_swing_low is not None and last_swing_low[1] > prev_swing_low[1]:
                    downtrend = False  # uptrend resumed
                    target_high_to_break = None
        # Fire ChoCH bullish when close breaks above target_high_to_break
        if downtrend and target_high_to_break is not None and c[i] > target_high_to_break:
            out[i] = True
            downtrend = False  # reset; structure flipped
            target_high_to_break = None
    return out

# ── RSI divergence signal ────────────────────────────────────────────────────
def sig_rsi_divergence(df, rsi_period=14, pivot_len=5):
    """Bullish divergence: price new low + RSI higher low.

    Track price swing lows AND RSI value at those points. If new swing low has
    price < prior swing low but RSI > RSI at prior swing low, fire signal.
    """
    c = df.close.values
    l = df.low.values
    r = rsi(df.close, rsi_period).values
    sz = len(df)
    pivot_low = find_pivots(l, pivot_len, pivot_len, is_high=False)
    out = np.zeros(sz, dtype=bool)
    last_low_price = None
    last_low_rsi = None
    last_low_idx = -1
    for i in range(sz):
        conf_i = i - pivot_len
        if conf_i >= 0 and pivot_low[conf_i]:
            new_price = l[conf_i]
            new_rsi = r[conf_i]
            if (last_low_price is not None and not np.isnan(new_rsi) and not np.isnan(last_low_rsi)):
                if new_price < last_low_price and new_rsi > last_low_rsi:
                    # Bullish divergence confirmed at conf_i; emit signal at i (current bar)
                    out[i] = True
            last_low_price = new_price
            last_low_rsi = new_rsi
            last_low_idx = conf_i
    return out

# ── Combined: ChoCH + RSI divergence within window ───────────────────────────
def sig_choch_rsi_combo(df, pivot_len=5, rsi_period=14, window=5):
    """Bullish ChoCH fires AND RSI bull-div occurred within last `window` bars."""
    choch = sig_choch(df, pivot_len)
    div = sig_rsi_divergence(df, rsi_period, pivot_len)
    sz = len(df)
    out = np.zeros(sz, dtype=bool)
    # Rolling-OR over div on prior `window` bars
    div_recent = np.zeros(sz, dtype=bool)
    for i in range(sz):
        lo = max(0, i - window)
        div_recent[i] = np.any(div[lo:i+1])
    out = choch & div_recent
    return out

# ── Backtest harness adapted to custom entry array ───────────────────────────
def backtest_custom(df, entries, eng_template, blowoff_pct=80.0):
    """Run backtest with FULL protections, custom entries array."""
    # Reuse backtest_engine by monkey-patching: easier to inline.
    # Use eng_template's protection params but our own entries
    lb   = eng_template["lookback"]
    hold = eng_template["hold_bars"]
    sl_m = eng_template["sl_atr_mult"]
    ap   = eng_template["atr_period"]
    cost = eng_template["round_trip_bp"] / 10_000.0
    rt_bp = eng_template["round_trip_bp"]
    arm  = eng_template["trail_arm_atr"]
    tdist= eng_template["trail_dist_atr"]
    tarm = eng_template["trail_tighten_atr"]
    tdis2= eng_template["trail_tighten_dist_atr"]

    hard_floor_bp   = -50.0
    early_kill_bp   = -25.0
    early_kill_mfe  = 15.0
    ratchet_start_bp = rt_bp
    be_arm_bp        = rt_bp + 10.0
    ratchet_lock_pct = 0.75
    prog2 = 0.85; prog3 = 0.90; prog4 = 0.95
    giveback_arm_bp = rt_bp + 20.0
    giveback_pct    = 0.15

    a = atr(df, ap).values
    c = df.close.values; h = df.high.values; l = df.low.values
    n = len(df)
    blowoff_skips = 0
    trades = []
    in_pos = False
    entry_px = entry_atr = sl_px = trail_stop = 0.0
    bars_held = 0
    high_since = 0.0
    trail_armed = False
    entry_i = 0
    peak_mfe_bp = 0.0

    for i in range(1, n):
        if in_pos:
            bars_held += 1
            high_since = max(high_since, h[i])
            mfe = high_since - entry_px
            mfe_bp = (high_since / entry_px - 1.0) * 1e4
            peak_mfe_bp = max(peak_mfe_bp, mfe_bp)
            if not trail_armed and mfe >= arm * entry_atr: trail_armed = True
            if trail_armed:
                dist = tdis2 if (tarm > 0 and mfe >= tarm * entry_atr) else tdist
                trail_stop = max(trail_stop, high_since - dist * entry_atr)
            if peak_mfe_bp < ratchet_start_bp:
                ratchet_px = entry_px * (1.0 + hard_floor_bp / 1e4)
            elif peak_mfe_bp < be_arm_bp:
                frac = (peak_mfe_bp - ratchet_start_bp) / max(1e-9, be_arm_bp - ratchet_start_bp)
                lock_bp = hard_floor_bp * (1.0 - frac)
                ratchet_px = entry_px * (1.0 + lock_bp / 1e4)
            else:
                if peak_mfe_bp < 100: lp = ratchet_lock_pct
                elif peak_mfe_bp < 200: lp = prog2
                elif peak_mfe_bp < 300: lp = prog3
                else: lp = prog4
                lock_bp = rt_bp + (peak_mfe_bp - be_arm_bp) * lp
                ratchet_px = entry_px * (1.0 + lock_bp / 1e4)
            stop = sl_px
            if trail_armed: stop = max(stop, trail_stop)
            stop = max(stop, ratchet_px)
            exit_px = None; reason = ""
            cur_low_bp = (l[i] / entry_px - 1.0) * 1e4
            if peak_mfe_bp < early_kill_mfe and cur_low_bp < early_kill_bp:
                exit_px = entry_px * (1.0 + early_kill_bp / 1e4); reason = "EARLY_KILL"
            if exit_px is None and peak_mfe_bp >= giveback_arm_bp:
                gtrig = peak_mfe_bp * (1.0 - giveback_pct)
                if cur_low_bp <= gtrig:
                    exit_px = entry_px * (1.0 + gtrig / 1e4); reason = "GIVEBACK"
            if exit_px is None and l[i] <= stop:
                exit_px = stop; reason = "SL" if not trail_armed else "TRAIL"
            if exit_px is None and bars_held >= hold:
                exit_px = c[i]; reason = "TIME"
            if exit_px is not None:
                gross = (exit_px - entry_px) / entry_px
                net = gross - cost
                trades.append((entry_i, i, gross, net, bars_held, peak_mfe_bp, reason))
                in_pos = False; trail_armed = False; trail_stop = 0.0; peak_mfe_bp = 0.0
            continue
        if entries[i] and not np.isnan(a[i]) and a[i] > 0:
            if blowoff_pct is not None and i >= lb:
                lb_close = c[i - lb]
                if lb_close > 0:
                    mom = (c[i] / lb_close - 1.0) * 100.0
                    if mom > blowoff_pct:
                        blowoff_skips += 1; continue
            in_pos = True
            entry_px = c[i]; entry_atr = a[i]; sl_px = entry_px - sl_m * entry_atr
            high_since = entry_px; bars_held = 0; entry_i = i; peak_mfe_bp = 0.0
    return trades, blowoff_skips

# ── Engine template (per TF) ─────────────────────────────────────────────────
def make_template(tf_secs):
    return dict(
        lookback=20, hold_bars=12, sl_atr_mult=3.0, atr_period=14,
        bb_k=2.0, rsi_threshold=30.0, round_trip_bp=20.0, max_history=64,
        trail_arm_atr=1.0, trail_dist_atr=0.4, trail_tighten_atr=3.0,
        trail_tighten_dist_atr=0.25,
    )

def main():
    rows = []
    for sym in SYMBOLS:
        h1 = load_h1(sym)
        if h1 is None: continue
        for tf in TFS:
            df = resample(h1, tf)
            if len(df) < 150: continue
            tf_secs = TF_TO_SECS[tf]
            bars_per_year = (365 * 24 * 3600) / tf_secs
            tmpl = make_template(tf_secs)

            # Run 3 signal variants
            for name, sig_fn in [
                ("CHOCH", lambda d: sig_choch(d, pivot_len=5)),
                ("RSIDIV", lambda d: sig_rsi_divergence(d, rsi_period=14, pivot_len=5)),
                ("CHOCH_RSIDIV", lambda d: sig_choch_rsi_combo(d, pivot_len=5, rsi_period=14, window=5)),
            ]:
                entries = sig_fn(df)
                trades, skips = backtest_custom(df, entries, tmpl, blowoff_pct=80.0)
                m = metrics(trades, len(df), bars_per_year)
                def col(k, dflt=0): return m.get(k, dflt) if m else dflt
                rows.append(dict(
                    sym=sym.upper().replace("USDT",""), tf=tf, signal=name,
                    n_signals=int(entries.sum()),
                    trades=col("trades"), pf=col("pf"), sharpe=col("sharpe"),
                    win=col("win"), mdd_bp=col("mdd_bp"), cum_bp=col("cum_bp"),
                    worst_bp=col("worst_bp"), avg_loss_bp=col("avg_loss_bp"),
                    avg_win_bp=col("avg_win_bp"),
                    n_sl=col("n_sl"), n_kill=col("n_kill"),
                    n_giveback=col("n_giveback"),
                    passes=passes(m),
                ))

    df_out = pd.DataFrame(rows)
    df_out["pf"] = df_out["pf"].replace([np.inf], 999)
    df_out.to_csv(OUT, index=False)

    for sig in ["CHOCH","RSIDIV","CHOCH_RSIDIV"]:
        sub = df_out[df_out.signal == sig]
        pass_ct = sub.passes.sum()
        print(f"{sig:14}  tested={len(sub):>3}  pass={pass_ct:>3}  avg_pf={sub[sub.passes].pf.mean():.2f}  avg_sharpe={sub[sub.passes].sharpe.mean():.2f}")

    print()
    print("=== TOP 25 SURVIVORS BY SHARPE ===")
    surv = df_out[df_out.passes].sort_values("sharpe", ascending=False)
    cols=["sym","tf","signal","n_signals","trades","pf","sharpe","cum_bp","mdd_bp","worst_bp","avg_loss_bp","avg_win_bp"]
    print(surv[cols].head(25).to_string(index=False, formatters={
        "pf":"{:.2f}".format,"sharpe":"{:.2f}".format,
        "cum_bp":"{:,.0f}".format,"mdd_bp":"{:,.0f}".format,
        "worst_bp":"{:.0f}".format,"avg_loss_bp":"{:.0f}".format,"avg_win_bp":"{:.0f}".format,
    }))

if __name__ == "__main__":
    main()
