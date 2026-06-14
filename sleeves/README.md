# Chimera Shadow Sleeves

Two OOS-validated edges (see Memory-Chimera CrossSectionalMomentum), SHADOW only:
- MOMENTUM: cross-sectional, lb30, top-3, rebal14d, inverse-vol, BTC>200d gate
- BREAKOUT: Donchian N40 + 2x volume, top-5, rebal14d, equal, BTC>200d gate

Combined 50/50: PF 1.43, WR 52%, maxDD 50%, Sharpe 1.54 (2020-2026, cost-incl).
Passes 2021/2023/2024 + 2025 holdout. Long-only spot. Flat in bear (macro gate).

run_daily.sh: update_data -> validate_dataset (hard gate) -> chimera_sleeves shadow.
Writes target weights to data/shadow_sleeves_ledger.csv. PLACES NO ORDERS.
Scheduled via chimera-sleeves.timer (daily 00:30 UTC). Going live = a separate,
explicit step (add an executor) after a shadow track record.
