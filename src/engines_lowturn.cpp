// S54m: LOW-TURNOVER trend sleeve. Wide stop (8 ATR), NO ratchet, long hold ->
// ~71% fewer trades than the standard config -> cost can't eat the edge. Validated
// (trustworthy fine-fill path, 38bp honest cost, BTC 11yr): 2.25x better than the
// standard config in BULL slices (+22623 vs +10056). Only trades in bulls via the
// macro 200d-MA gate + BULL_TREND regime gate, where it wins; the bears it is bad
// at are excluded. Ratchet/floor params set by the S45-FLOOR "LT-" exemption in
// main.cpp (ratchet OFF, be_arm OFF, floor -800). Shadow.
// D1 (86400) on the 6 most-liquid symbols + 2 H12 (43200) for a touch more breadth.

#define LT_CFG(VAR, SYM, TAG, TF, RT) \
  chimera::EdgeEngine::Config VAR##_cfg{ \
    .symbol=SYM, .tag=TAG, .kind=chimera::StrategyKind::TSMOM, \
    .tf_secs=TF, .lookback=10, .hold_bars=200, .sl_atr_mult=8.0, .atr_period=14, \
    .round_trip_bp=RT, .max_history=64, \
    .trail_arm_atr=3.0, .trail_dist_atr=2.0, .trail_tighten_atr=0.0, .trail_tighten_dist_atr=2.0, \
  }; \
  chimera::EdgeEngine VAR(VAR##_cfg); \
  wire_engine(VAR);

LT_CFG(lt_btc_d1,  "btcusdt", "LT-BTC-TSMOM-D1",  86400, 17)
LT_CFG(lt_eth_d1,  "ethusdt", "LT-ETH-TSMOM-D1",  86400, 17)
LT_CFG(lt_sol_d1,  "solusdt", "LT-SOL-TSMOM-D1",  86400, 20)
LT_CFG(lt_bnb_d1,  "bnbusdt", "LT-BNB-TSMOM-D1",  86400, 20)
LT_CFG(lt_link_d1, "linkusdt","LT-LINK-TSMOM-D1", 86400, 20)
LT_CFG(lt_doge_d1, "dogeusdt","LT-DOGE-TSMOM-D1", 86400, 20)
LT_CFG(lt_btc_h12, "btcusdt", "LT-BTC-TSMOM-H12", 43200, 17)
LT_CFG(lt_sol_h12, "solusdt", "LT-SOL-TSMOM-H12", 43200, 20)
// S56 expansion: BULL-slice validated (expansion_sweep.cpp lt mode, fine-fill D1,
// 38bp cost): NEAR n=7 pos=6 +34408bp both halves+; XRP n=8 pos=6 +8077 both
// halves+; UNI n=4 pos=3 +7553 both halves+. Macro-gated like the rest of sleeve.
LT_CFG(lt_near_d1, "nearusdt","LT-NEAR-TSMOM-D1", 86400, 20)
LT_CFG(lt_xrp_d1,  "xrpusdt", "LT-XRP-TSMOM-D1",  86400, 20)
LT_CFG(lt_uni_d1,  "uniusdt", "LT-UNI-TSMOM-D1",  86400, 20)

#undef LT_CFG
