// S43b-HOLDOUT — 142 engines from re-discovery with TRUE held-out forward validation.
// Generated 2026-05-29 after the S38-S42 cull.
//
// Protocol:
//   1. backtest_harness.cpp --discover ran with --end-days-ago 180
//      so the optimizer was BLIND to the last 180 days of data.
//      Discover window: [-545d, -180d] (365d slice).
//   2. Top config per (symbol, kind, tf) by IS PF >= 1.5, n >= 20, bp > 0.
//   3. Each top config re-validated on --last-days 180 (TRUE forward holdout).
//   4. VIABLE gate applied:
//        OOS PF >= 1.5  AND  OOS Sharpe >= 2.0  AND  OOS n >= 50
//        OOS bp / OOS MaxDD >= 2.0
//        OOS PF >= 0.5 * IS PF (no edge collapse)
//        IS PF >= 1.5
//   5. Confirmed --end-days-ago flag took effect via independent re-run
//      (EIGEN-TSMOM-H12 IS n=148 vs OOS n=70 — disjoint windows).
//
// Backup: /Users/jo/Chimera_Baselines/pre_cull_20260529_162344
// Verdict CSV: /tmp/proper_verdict.csv | Viable CSV: /tmp/viable_142.tsv
//
// ─── CONFIG + ENGINE + WIRE ──────────────────────────────────────────────
// APT-ICHI-H6  IS_PF=2.18 n=145  OOS_PF=2.33 n=54 bp=+4755 dd=2182
chimera::EdgeEngine::Config s43b_aptusdt_ichimoku_21600_6_24_cfg{
    .symbol="aptusdt", .tag="APT-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_aptusdt_ichimoku_21600_6_24(s43b_aptusdt_ichimoku_21600_6_24_cfg);
wire_engine(s43b_aptusdt_ichimoku_21600_6_24);

// APT-TSMOM-H3  IS_PF=1.68 n=881  OOS_PF=1.71 n=392 bp=+13459 dd=1602
chimera::EdgeEngine::Config s43b_aptusdt_tsmom_10800_45_3_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_aptusdt_tsmom_10800_45_3(s43b_aptusdt_tsmom_10800_45_3_cfg);
wire_engine(s43b_aptusdt_tsmom_10800_45_3);

// APT-TSMOM-H6  IS_PF=2.33 n=373  OOS_PF=2.26 n=165 bp=+10713 dd=2192
chimera::EdgeEngine::Config s43b_aptusdt_tsmom_21600_45_5_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_aptusdt_tsmom_21600_45_5(s43b_aptusdt_tsmom_21600_45_5_cfg);
wire_engine(s43b_aptusdt_tsmom_21600_45_5);

// ARB-ICHI-H3  IS_PF=1.91 n=538  OOS_PF=1.51 n=213 bp=+5222 dd=1404
chimera::EdgeEngine::Config s43b_arbusdt_ichimoku_10800_6_5_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arbusdt_ichimoku_10800_6_5(s43b_arbusdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_arbusdt_ichimoku_10800_6_5);

// ARB-ICHI-H4  IS_PF=2.05 n=332  OOS_PF=1.76 n=156 bp=+6263 dd=1556
chimera::EdgeEngine::Config s43b_arbusdt_ichimoku_14400_6_3_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arbusdt_ichimoku_14400_6_3(s43b_arbusdt_ichimoku_14400_6_3_cfg);
wire_engine(s43b_arbusdt_ichimoku_14400_6_3);

// ARB-TSMOM-H2  IS_PF=1.71 n=1382  OOS_PF=1.76 n=588 bp=+16423 dd=1265
chimera::EdgeEngine::Config s43b_arbusdt_tsmom_7200_45_3_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arbusdt_tsmom_7200_45_3(s43b_arbusdt_tsmom_7200_45_3_cfg);
wire_engine(s43b_arbusdt_tsmom_7200_45_3);

// ARB-TSMOM-H3  IS_PF=1.94 n=919  OOS_PF=2.00 n=401 bp=+16277 dd=1320
chimera::EdgeEngine::Config s43b_arbusdt_tsmom_10800_45_8_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arbusdt_tsmom_10800_45_8(s43b_arbusdt_tsmom_10800_45_8_cfg);
wire_engine(s43b_arbusdt_tsmom_10800_45_8);

// ARB-TSMOM-H4  IS_PF=2.45 n=698  OOS_PF=1.94 n=317 bp=+14013 dd=1311
chimera::EdgeEngine::Config s43b_arbusdt_tsmom_14400_30_12_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arbusdt_tsmom_14400_30_12(s43b_arbusdt_tsmom_14400_30_12_cfg);
wire_engine(s43b_arbusdt_tsmom_14400_30_12);

// ARKM-ICHI-H4  IS_PF=1.81 n=289  OOS_PF=2.62 n=101 bp=+10837 dd=1259
chimera::EdgeEngine::Config s43b_arkmusdt_ichimoku_14400_6_3_cfg{
    .symbol="arkmusdt", .tag="ARKM-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arkmusdt_ichimoku_14400_6_3(s43b_arkmusdt_ichimoku_14400_6_3_cfg);
wire_engine(s43b_arkmusdt_ichimoku_14400_6_3);

// ARKM-TSMOM-H12  IS_PF=2.48 n=147  OOS_PF=2.07 n=63 bp=+12100 dd=3151
chimera::EdgeEngine::Config s43b_arkmusdt_tsmom_43200_18_12_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arkmusdt_tsmom_43200_18_12(s43b_arkmusdt_tsmom_43200_18_12_cfg);
wire_engine(s43b_arkmusdt_tsmom_43200_18_12);

// ARKM-TSMOM-H4  IS_PF=1.65 n=700  OOS_PF=1.57 n=300 bp=+13502 dd=2277
chimera::EdgeEngine::Config s43b_arkmusdt_tsmom_14400_18_3_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arkmusdt_tsmom_14400_18_3(s43b_arkmusdt_tsmom_14400_18_3_cfg);
wire_engine(s43b_arkmusdt_tsmom_14400_18_3);

// ARKM-TSMOM-H6  IS_PF=2.15 n=345  OOS_PF=1.77 n=169 bp=+10382 dd=2354
chimera::EdgeEngine::Config s43b_arkmusdt_tsmom_21600_60_24_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arkmusdt_tsmom_21600_60_24(s43b_arkmusdt_tsmom_21600_60_24_cfg);
wire_engine(s43b_arkmusdt_tsmom_21600_60_24);

// ARKM-TSMOM-H8  IS_PF=1.87 n=271  OOS_PF=2.04 n=136 bp=+12742 dd=1983
chimera::EdgeEngine::Config s43b_arkmusdt_tsmom_28800_45_5_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arkmusdt_tsmom_28800_45_5(s43b_arkmusdt_tsmom_28800_45_5_cfg);
wire_engine(s43b_arkmusdt_tsmom_28800_45_5);

// ARKM-WILLR-H6  IS_PF=2.43 n=103  OOS_PF=3.06 n=52 bp=+6270 dd=1123
chimera::EdgeEngine::Config s43b_arkmusdt_williams_r_21600_6_24_cfg{
    .symbol="arkmusdt", .tag="ARKM-WILLR-H6", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=21600, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_arkmusdt_williams_r_21600_6_24(s43b_arkmusdt_williams_r_21600_6_24_cfg);
wire_engine(s43b_arkmusdt_williams_r_21600_6_24);

// BOME-BOPB-H2  IS_PF=2.52 n=113  OOS_PF=1.88 n=52 bp=+5533 dd=2109
chimera::EdgeEngine::Config s43b_bomeusdt_breakout_pullback_7200_12_18_cfg{
    .symbol="bomeusdt", .tag="BOME-BOPB-H2", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=7200, .lookback=12, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_bomeusdt_breakout_pullback_7200_12_18(s43b_bomeusdt_breakout_pullback_7200_12_18_cfg);
wire_engine(s43b_bomeusdt_breakout_pullback_7200_12_18);

// BOME-ICHI-H1  IS_PF=1.51 n=902  OOS_PF=1.62 n=392 bp=+10628 dd=1892
chimera::EdgeEngine::Config s43b_bomeusdt_ichimoku_3600_6_18_cfg{
    .symbol="bomeusdt", .tag="BOME-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_bomeusdt_ichimoku_3600_6_18(s43b_bomeusdt_ichimoku_3600_6_18_cfg);
wire_engine(s43b_bomeusdt_ichimoku_3600_6_18);

// BOME-ICHI-H3  IS_PF=1.73 n=288  OOS_PF=2.54 n=172 bp=+15070 dd=1715
chimera::EdgeEngine::Config s43b_bomeusdt_ichimoku_10800_6_5_cfg{
    .symbol="bomeusdt", .tag="BOME-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_bomeusdt_ichimoku_10800_6_5(s43b_bomeusdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_bomeusdt_ichimoku_10800_6_5);

// BOME-TSMOM-H12  IS_PF=2.36 n=185  OOS_PF=2.05 n=113 bp=+15913 dd=2135
chimera::EdgeEngine::Config s43b_bomeusdt_tsmom_43200_30_3_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_bomeusdt_tsmom_43200_30_3(s43b_bomeusdt_tsmom_43200_30_3_cfg);
wire_engine(s43b_bomeusdt_tsmom_43200_30_3);

// BOME-TSMOM-H3  IS_PF=2.16 n=777  OOS_PF=1.60 n=367 bp=+16708 dd=4185
chimera::EdgeEngine::Config s43b_bomeusdt_tsmom_10800_12_5_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=12, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_bomeusdt_tsmom_10800_12_5(s43b_bomeusdt_tsmom_10800_12_5_cfg);
wire_engine(s43b_bomeusdt_tsmom_10800_12_5);

// BOME-TSMOM-H4  IS_PF=1.80 n=649  OOS_PF=2.48 n=309 bp=+26757 dd=2552
chimera::EdgeEngine::Config s43b_bomeusdt_tsmom_14400_30_3_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_bomeusdt_tsmom_14400_30_3(s43b_bomeusdt_tsmom_14400_30_3_cfg);
wire_engine(s43b_bomeusdt_tsmom_14400_30_3);

// BOME-TSMOM-H8  IS_PF=2.17 n=326  OOS_PF=1.67 n=174 bp=+14781 dd=3933
chimera::EdgeEngine::Config s43b_bomeusdt_tsmom_28800_18_5_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_bomeusdt_tsmom_28800_18_5(s43b_bomeusdt_tsmom_28800_18_5_cfg);
wire_engine(s43b_bomeusdt_tsmom_28800_18_5);

// EIGEN-ICHI-H2  IS_PF=1.67 n=782  OOS_PF=1.59 n=256 bp=+8367 dd=1593
chimera::EdgeEngine::Config s43b_eigenusdt_ichimoku_7200_6_12_cfg{
    .symbol="eigenusdt", .tag="EIGEN-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_eigenusdt_ichimoku_7200_6_12(s43b_eigenusdt_ichimoku_7200_6_12_cfg);
wire_engine(s43b_eigenusdt_ichimoku_7200_6_12);

// EIGEN-ICHI-H3  IS_PF=1.80 n=502  OOS_PF=1.67 n=153 bp=+7054 dd=1355
chimera::EdgeEngine::Config s43b_eigenusdt_ichimoku_10800_6_5_cfg{
    .symbol="eigenusdt", .tag="EIGEN-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_eigenusdt_ichimoku_10800_6_5(s43b_eigenusdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_eigenusdt_ichimoku_10800_6_5);

// EIGEN-ICHI-H4  IS_PF=1.73 n=346  OOS_PF=1.97 n=104 bp=+7754 dd=2245
chimera::EdgeEngine::Config s43b_eigenusdt_ichimoku_14400_6_5_cfg{
    .symbol="eigenusdt", .tag="EIGEN-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_eigenusdt_ichimoku_14400_6_5(s43b_eigenusdt_ichimoku_14400_6_5_cfg);
wire_engine(s43b_eigenusdt_ichimoku_14400_6_5);

// EIGEN-TSMOM-H12  IS_PF=4.61 n=148  OOS_PF=5.58 n=70 bp=+16721 dd=2863
chimera::EdgeEngine::Config s43b_eigenusdt_tsmom_43200_45_18_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_eigenusdt_tsmom_43200_45_18(s43b_eigenusdt_tsmom_43200_45_18_cfg);
wire_engine(s43b_eigenusdt_tsmom_43200_45_18);

// EIGEN-TSMOM-H4  IS_PF=2.08 n=698  OOS_PF=2.26 n=278 bp=+19602 dd=1530
chimera::EdgeEngine::Config s43b_eigenusdt_tsmom_14400_45_8_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_eigenusdt_tsmom_14400_45_8(s43b_eigenusdt_tsmom_14400_45_8_cfg);
wire_engine(s43b_eigenusdt_tsmom_14400_45_8);

// EIGEN-TSMOM-H6  IS_PF=2.14 n=442  OOS_PF=2.05 n=183 bp=+14833 dd=4832
chimera::EdgeEngine::Config s43b_eigenusdt_tsmom_21600_45_3_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_eigenusdt_tsmom_21600_45_3(s43b_eigenusdt_tsmom_21600_45_3_cfg);
wire_engine(s43b_eigenusdt_tsmom_21600_45_3);

// EIGEN-TSMOM-H8  IS_PF=2.38 n=256  OOS_PF=2.16 n=109 bp=+12584 dd=3335
chimera::EdgeEngine::Config s43b_eigenusdt_tsmom_28800_12_24_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=12, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_eigenusdt_tsmom_28800_12_24(s43b_eigenusdt_tsmom_28800_12_24_cfg);
wire_engine(s43b_eigenusdt_tsmom_28800_12_24);

// ENA-ICHI-H2  IS_PF=2.06 n=627  OOS_PF=2.01 n=244 bp=+11067 dd=1284
chimera::EdgeEngine::Config s43b_enausdt_ichimoku_7200_6_3_cfg{
    .symbol="enausdt", .tag="ENA-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_enausdt_ichimoku_7200_6_3(s43b_enausdt_ichimoku_7200_6_3_cfg);
wire_engine(s43b_enausdt_ichimoku_7200_6_3);

// ENA-TSMOM-H12  IS_PF=5.35 n=142  OOS_PF=3.58 n=50 bp=+7924 dd=2587
chimera::EdgeEngine::Config s43b_enausdt_tsmom_43200_60_24_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_enausdt_tsmom_43200_60_24(s43b_enausdt_tsmom_43200_60_24_cfg);
wire_engine(s43b_enausdt_tsmom_43200_60_24);

// ENA-TSMOM-H2  IS_PF=1.85 n=1199  OOS_PF=1.64 n=489 bp=+14143 dd=1793
chimera::EdgeEngine::Config s43b_enausdt_tsmom_7200_30_3_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_enausdt_tsmom_7200_30_3(s43b_enausdt_tsmom_7200_30_3_cfg);
wire_engine(s43b_enausdt_tsmom_7200_30_3);

// ENA-TSMOM-H4  IS_PF=2.17 n=479  OOS_PF=1.54 n=195 bp=+8152 dd=3587
chimera::EdgeEngine::Config s43b_enausdt_tsmom_14400_30_8_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_enausdt_tsmom_14400_30_8(s43b_enausdt_tsmom_14400_30_8_cfg);
wire_engine(s43b_enausdt_tsmom_14400_30_8);

// ENA-TSMOM-H6  IS_PF=2.71 n=310  OOS_PF=2.29 n=148 bp=+11885 dd=2791
chimera::EdgeEngine::Config s43b_enausdt_tsmom_21600_60_8_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_enausdt_tsmom_21600_60_8(s43b_enausdt_tsmom_21600_60_8_cfg);
wire_engine(s43b_enausdt_tsmom_21600_60_8);

// ENA-TSMOM-H8  IS_PF=2.52 n=293  OOS_PF=1.71 n=116 bp=+8488 dd=3285
chimera::EdgeEngine::Config s43b_enausdt_tsmom_28800_30_3_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_enausdt_tsmom_28800_30_3(s43b_enausdt_tsmom_28800_30_3_cfg);
wire_engine(s43b_enausdt_tsmom_28800_30_3);

// ETHFI-TSMOM-H2  IS_PF=1.94 n=1299  OOS_PF=1.61 n=567 bp=+13464 dd=1688
chimera::EdgeEngine::Config s43b_ethfiusdt_tsmom_7200_60_5_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_ethfiusdt_tsmom_7200_60_5(s43b_ethfiusdt_tsmom_7200_60_5_cfg);
wire_engine(s43b_ethfiusdt_tsmom_7200_60_5);

// ETHFI-TSMOM-H3  IS_PF=2.25 n=901  OOS_PF=1.53 n=410 bp=+12101 dd=2742
chimera::EdgeEngine::Config s43b_ethfiusdt_tsmom_10800_45_3_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_ethfiusdt_tsmom_10800_45_3(s43b_ethfiusdt_tsmom_10800_45_3_cfg);
wire_engine(s43b_ethfiusdt_tsmom_10800_45_3);

// ETHFI-TSMOM-H6  IS_PF=2.40 n=422  OOS_PF=2.03 n=188 bp=+12936 dd=1190
chimera::EdgeEngine::Config s43b_ethfiusdt_tsmom_21600_60_3_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_ethfiusdt_tsmom_21600_60_3(s43b_ethfiusdt_tsmom_21600_60_3_cfg);
wire_engine(s43b_ethfiusdt_tsmom_21600_60_3);

// FLOKI-ICHI-H4  IS_PF=1.68 n=282  OOS_PF=1.65 n=145 bp=+5564 dd=1662
chimera::EdgeEngine::Config s43b_flokiusdt_ichimoku_14400_6_5_cfg{
    .symbol="flokiusdt", .tag="FLOKI-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_flokiusdt_ichimoku_14400_6_5(s43b_flokiusdt_ichimoku_14400_6_5_cfg);
wire_engine(s43b_flokiusdt_ichimoku_14400_6_5);

// FLOKI-TSMOM-H3  IS_PF=2.14 n=825  OOS_PF=1.83 n=368 bp=+14127 dd=1499
chimera::EdgeEngine::Config s43b_flokiusdt_tsmom_10800_60_3_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_flokiusdt_tsmom_10800_60_3(s43b_flokiusdt_tsmom_10800_60_3_cfg);
wire_engine(s43b_flokiusdt_tsmom_10800_60_3);

// FLOKI-TSMOM-H4  IS_PF=2.18 n=595  OOS_PF=2.37 n=301 bp=+16624 dd=1234
chimera::EdgeEngine::Config s43b_flokiusdt_tsmom_14400_60_5_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_flokiusdt_tsmom_14400_60_5(s43b_flokiusdt_tsmom_14400_60_5_cfg);
wire_engine(s43b_flokiusdt_tsmom_14400_60_5);

// FLOKI-TSMOM-H6  IS_PF=2.66 n=366  OOS_PF=2.11 n=161 bp=+10726 dd=2142
chimera::EdgeEngine::Config s43b_flokiusdt_tsmom_21600_30_5_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_flokiusdt_tsmom_21600_30_5(s43b_flokiusdt_tsmom_21600_30_5_cfg);
wire_engine(s43b_flokiusdt_tsmom_21600_30_5);

// FLOKI-TSMOM-H8  IS_PF=2.83 n=305  OOS_PF=1.98 n=164 bp=+11092 dd=1162
chimera::EdgeEngine::Config s43b_flokiusdt_tsmom_28800_30_5_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_flokiusdt_tsmom_28800_30_5(s43b_flokiusdt_tsmom_28800_30_5_cfg);
wire_engine(s43b_flokiusdt_tsmom_28800_30_5);

// JTO-ICHI-H3  IS_PF=1.50 n=387  OOS_PF=1.82 n=230 bp=+11446 dd=1963
chimera::EdgeEngine::Config s43b_jtousdt_ichimoku_10800_6_3_cfg{
    .symbol="jtousdt", .tag="JTO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_ichimoku_10800_6_3(s43b_jtousdt_ichimoku_10800_6_3_cfg);
wire_engine(s43b_jtousdt_ichimoku_10800_6_3);

// JTO-ICHI-H8  IS_PF=1.51 n=110  OOS_PF=2.67 n=56 bp=+8942 dd=925
chimera::EdgeEngine::Config s43b_jtousdt_ichimoku_28800_6_5_cfg{
    .symbol="jtousdt", .tag="JTO-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_ichimoku_28800_6_5(s43b_jtousdt_ichimoku_28800_6_5_cfg);
wire_engine(s43b_jtousdt_ichimoku_28800_6_5);

// JTO-TSMOM-H1  IS_PF=1.59 n=2369  OOS_PF=1.80 n=1310 bp=+39688 dd=1389
chimera::EdgeEngine::Config s43b_jtousdt_tsmom_3600_60_12_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_tsmom_3600_60_12(s43b_jtousdt_tsmom_3600_60_12_cfg);
wire_engine(s43b_jtousdt_tsmom_3600_60_12);

// JTO-TSMOM-H12  IS_PF=2.16 n=217  OOS_PF=4.12 n=128 bp=+29519 dd=1916
chimera::EdgeEngine::Config s43b_jtousdt_tsmom_43200_18_5_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_tsmom_43200_18_5(s43b_jtousdt_tsmom_43200_18_5_cfg);
wire_engine(s43b_jtousdt_tsmom_43200_18_5);

// JTO-TSMOM-H2  IS_PF=1.92 n=1297  OOS_PF=1.80 n=712 bp=+29219 dd=1816
chimera::EdgeEngine::Config s43b_jtousdt_tsmom_7200_30_3_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_tsmom_7200_30_3(s43b_jtousdt_tsmom_7200_30_3_cfg);
wire_engine(s43b_jtousdt_tsmom_7200_30_3);

// JTO-TSMOM-H3  IS_PF=1.84 n=839  OOS_PF=2.00 n=512 bp=+30555 dd=2155
chimera::EdgeEngine::Config s43b_jtousdt_tsmom_10800_60_3_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_tsmom_10800_60_3(s43b_jtousdt_tsmom_10800_60_3_cfg);
wire_engine(s43b_jtousdt_tsmom_10800_60_3);

// JTO-TSMOM-H4  IS_PF=2.14 n=652  OOS_PF=3.13 n=394 bp=+40774 dd=1815
chimera::EdgeEngine::Config s43b_jtousdt_tsmom_14400_60_3_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_tsmom_14400_60_3(s43b_jtousdt_tsmom_14400_60_3_cfg);
wire_engine(s43b_jtousdt_tsmom_14400_60_3);

// JTO-TSMOM-H6  IS_PF=1.80 n=410  OOS_PF=2.57 n=265 bp=+27157 dd=1530
chimera::EdgeEngine::Config s43b_jtousdt_tsmom_21600_45_5_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_tsmom_21600_45_5(s43b_jtousdt_tsmom_21600_45_5_cfg);
// S44-CULL: wire_engine(s43b_jtousdt_tsmom_21600_45_5);

// JTO-TSMOM-H8  IS_PF=2.42 n=345  OOS_PF=3.63 n=203 bp=+35721 dd=1821
chimera::EdgeEngine::Config s43b_jtousdt_tsmom_28800_30_3_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jtousdt_tsmom_28800_30_3(s43b_jtousdt_tsmom_28800_30_3_cfg);
wire_engine(s43b_jtousdt_tsmom_28800_30_3);

// JUP-ICHI-H1  IS_PF=1.57 n=1578  OOS_PF=1.59 n=681 bp=+13130 dd=1674
chimera::EdgeEngine::Config s43b_jupusdt_ichimoku_3600_6_3_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_ichimoku_3600_6_3(s43b_jupusdt_ichimoku_3600_6_3_cfg);
wire_engine(s43b_jupusdt_ichimoku_3600_6_3);

// JUP-ICHI-H2  IS_PF=2.01 n=674  OOS_PF=1.68 n=293 bp=+8802 dd=1810
chimera::EdgeEngine::Config s43b_jupusdt_ichimoku_7200_6_3_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_ichimoku_7200_6_3(s43b_jupusdt_ichimoku_7200_6_3_cfg);
wire_engine(s43b_jupusdt_ichimoku_7200_6_3);

// JUP-ICHI-H3  IS_PF=2.06 n=488  OOS_PF=1.54 n=187 bp=+5530 dd=2092
chimera::EdgeEngine::Config s43b_jupusdt_ichimoku_10800_6_3_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_ichimoku_10800_6_3(s43b_jupusdt_ichimoku_10800_6_3_cfg);
wire_engine(s43b_jupusdt_ichimoku_10800_6_3);

// JUP-TSMOM-D1  IS_PF=3.08 n=118  OOS_PF=1.71 n=61 bp=+6884 dd=3405
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_86400_18_3_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_86400_18_3(s43b_jupusdt_tsmom_86400_18_3_cfg);
wire_engine(s43b_jupusdt_tsmom_86400_18_3);

// JUP-TSMOM-H1  IS_PF=1.75 n=2611  OOS_PF=1.60 n=1195 bp=+22450 dd=1692
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_3600_60_8_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_3600_60_8(s43b_jupusdt_tsmom_3600_60_8_cfg);
wire_engine(s43b_jupusdt_tsmom_3600_60_8);

// JUP-TSMOM-H12  IS_PF=2.55 n=194  OOS_PF=2.00 n=116 bp=+10506 dd=1572
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_43200_60_3_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_43200_60_3(s43b_jupusdt_tsmom_43200_60_3_cfg);
wire_engine(s43b_jupusdt_tsmom_43200_60_3);

// JUP-TSMOM-H2  IS_PF=2.17 n=1336  OOS_PF=1.58 n=677 bp=+15668 dd=1571
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_7200_60_3_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_7200_60_3(s43b_jupusdt_tsmom_7200_60_3_cfg);
wire_engine(s43b_jupusdt_tsmom_7200_60_3);

// JUP-TSMOM-H3  IS_PF=2.25 n=963  OOS_PF=1.90 n=457 bp=+19128 dd=2147
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_10800_30_3_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_10800_30_3(s43b_jupusdt_tsmom_10800_30_3_cfg);
wire_engine(s43b_jupusdt_tsmom_10800_30_3);

// JUP-TSMOM-H4  IS_PF=2.29 n=713  OOS_PF=2.00 n=349 bp=+16723 dd=3194
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_14400_60_3_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_14400_60_3(s43b_jupusdt_tsmom_14400_60_3_cfg);
wire_engine(s43b_jupusdt_tsmom_14400_60_3);

// JUP-TSMOM-H6  IS_PF=2.24 n=492  OOS_PF=1.67 n=235 bp=+12028 dd=2616
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_21600_18_3_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_21600_18_3(s43b_jupusdt_tsmom_21600_18_3_cfg);
wire_engine(s43b_jupusdt_tsmom_21600_18_3);

// JUP-TSMOM-H8  IS_PF=2.47 n=311  OOS_PF=1.78 n=144 bp=+10664 dd=2276
chimera::EdgeEngine::Config s43b_jupusdt_tsmom_28800_6_5_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=6, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_jupusdt_tsmom_28800_6_5(s43b_jupusdt_tsmom_28800_6_5_cfg);
wire_engine(s43b_jupusdt_tsmom_28800_6_5);

// ONDO-ICHI-H2  IS_PF=1.70 n=61  OOS_PF=1.95 n=185 bp=+8432 dd=2046
chimera::EdgeEngine::Config s43b_ondousdt_ichimoku_7200_6_24_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_ondousdt_ichimoku_7200_6_24(s43b_ondousdt_ichimoku_7200_6_24_cfg);
wire_engine(s43b_ondousdt_ichimoku_7200_6_24);

// ONDO-ICHI-H3  IS_PF=2.46 n=48  OOS_PF=2.04 n=122 bp=+7427 dd=1923
chimera::EdgeEngine::Config s43b_ondousdt_ichimoku_10800_6_8_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_ondousdt_ichimoku_10800_6_8(s43b_ondousdt_ichimoku_10800_6_8_cfg);
wire_engine(s43b_ondousdt_ichimoku_10800_6_8);

// OP-ICHI-H2  IS_PF=1.58 n=782  OOS_PF=1.54 n=328 bp=+7663 dd=1297
chimera::EdgeEngine::Config s43b_opusdt_ichimoku_7200_6_3_cfg{
    .symbol="opusdt", .tag="OP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_opusdt_ichimoku_7200_6_3(s43b_opusdt_ichimoku_7200_6_3_cfg);
wire_engine(s43b_opusdt_ichimoku_7200_6_3);

// OP-ICHI-H8  IS_PF=2.11 n=96  OOS_PF=2.54 n=65 bp=+6898 dd=1363
chimera::EdgeEngine::Config s43b_opusdt_ichimoku_28800_6_5_cfg{
    .symbol="opusdt", .tag="OP-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_opusdt_ichimoku_28800_6_5(s43b_opusdt_ichimoku_28800_6_5_cfg);
wire_engine(s43b_opusdt_ichimoku_28800_6_5);

// OP-TSMOM-H12  IS_PF=2.01 n=221  OOS_PF=1.60 n=113 bp=+6370 dd=1954
chimera::EdgeEngine::Config s43b_opusdt_tsmom_43200_18_5_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_opusdt_tsmom_43200_18_5(s43b_opusdt_tsmom_43200_18_5_cfg);
wire_engine(s43b_opusdt_tsmom_43200_18_5);

// OP-TSMOM-H2  IS_PF=1.75 n=1362  OOS_PF=1.52 n=602 bp=+12912 dd=2199
chimera::EdgeEngine::Config s43b_opusdt_tsmom_7200_30_12_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=30, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_opusdt_tsmom_7200_30_12(s43b_opusdt_tsmom_7200_30_12_cfg);
wire_engine(s43b_opusdt_tsmom_7200_30_12);

// OP-TSMOM-H3  IS_PF=1.79 n=815  OOS_PF=1.64 n=423 bp=+11151 dd=1666
chimera::EdgeEngine::Config s43b_opusdt_tsmom_10800_60_3_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_opusdt_tsmom_10800_60_3(s43b_opusdt_tsmom_10800_60_3_cfg);
wire_engine(s43b_opusdt_tsmom_10800_60_3);

// OP-TSMOM-H4  IS_PF=1.97 n=634  OOS_PF=1.86 n=298 bp=+12439 dd=1319
chimera::EdgeEngine::Config s43b_opusdt_tsmom_14400_60_3_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_opusdt_tsmom_14400_60_3(s43b_opusdt_tsmom_14400_60_3_cfg);
wire_engine(s43b_opusdt_tsmom_14400_60_3);

// OP-TSMOM-H8  IS_PF=2.42 n=245  OOS_PF=2.20 n=146 bp=+11426 dd=1266
chimera::EdgeEngine::Config s43b_opusdt_tsmom_28800_60_5_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_opusdt_tsmom_28800_60_5(s43b_opusdt_tsmom_28800_60_5_cfg);
wire_engine(s43b_opusdt_tsmom_28800_60_5);

// PEPE-ICHI-H3  IS_PF=2.47 n=471  OOS_PF=1.81 n=216 bp=+10476 dd=1398
chimera::EdgeEngine::Config s43b_pepeusdt_ichimoku_10800_6_5_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pepeusdt_ichimoku_10800_6_5(s43b_pepeusdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_pepeusdt_ichimoku_10800_6_5);

// PEPE-TSMOM-H12  IS_PF=3.01 n=142  OOS_PF=3.01 n=71 bp=+9798 dd=1812
chimera::EdgeEngine::Config s43b_pepeusdt_tsmom_43200_60_12_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pepeusdt_tsmom_43200_60_12(s43b_pepeusdt_tsmom_43200_60_12_cfg);
wire_engine(s43b_pepeusdt_tsmom_43200_60_12);

// PEPE-TSMOM-H3  IS_PF=2.28 n=859  OOS_PF=1.59 n=439 bp=+12800 dd=2296
chimera::EdgeEngine::Config s43b_pepeusdt_tsmom_10800_60_3_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pepeusdt_tsmom_10800_60_3(s43b_pepeusdt_tsmom_10800_60_3_cfg);
wire_engine(s43b_pepeusdt_tsmom_10800_60_3);

// PEPE-TSMOM-H4  IS_PF=2.24 n=656  OOS_PF=2.04 n=319 bp=+17284 dd=2306
chimera::EdgeEngine::Config s43b_pepeusdt_tsmom_14400_60_8_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pepeusdt_tsmom_14400_60_8(s43b_pepeusdt_tsmom_14400_60_8_cfg);
wire_engine(s43b_pepeusdt_tsmom_14400_60_8);

// PEPE-TSMOM-H6  IS_PF=2.08 n=431  OOS_PF=2.25 n=235 bp=+18668 dd=2143
chimera::EdgeEngine::Config s43b_pepeusdt_tsmom_21600_45_5_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pepeusdt_tsmom_21600_45_5(s43b_pepeusdt_tsmom_21600_45_5_cfg);
wire_engine(s43b_pepeusdt_tsmom_21600_45_5);

// PYTH-ICHI-H6  IS_PF=1.77 n=147  OOS_PF=1.55 n=91 bp=+4045 dd=1547
chimera::EdgeEngine::Config s43b_pythusdt_ichimoku_21600_6_3_cfg{
    .symbol="pythusdt", .tag="PYTH-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_ichimoku_21600_6_3(s43b_pythusdt_ichimoku_21600_6_3_cfg);
wire_engine(s43b_pythusdt_ichimoku_21600_6_3);

// PYTH-ICHI-H8  IS_PF=1.62 n=98  OOS_PF=1.86 n=84 bp=+5553 dd=1386
chimera::EdgeEngine::Config s43b_pythusdt_ichimoku_28800_6_5_cfg{
    .symbol="pythusdt", .tag="PYTH-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_ichimoku_28800_6_5(s43b_pythusdt_ichimoku_28800_6_5_cfg);
wire_engine(s43b_pythusdt_ichimoku_28800_6_5);

// PYTH-TSMOM-H12  IS_PF=2.57 n=154  OOS_PF=2.70 n=94 bp=+11816 dd=1613
chimera::EdgeEngine::Config s43b_pythusdt_tsmom_43200_45_12_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_tsmom_43200_45_12(s43b_pythusdt_tsmom_43200_45_12_cfg);
wire_engine(s43b_pythusdt_tsmom_43200_45_12);

// PYTH-TSMOM-H2  IS_PF=1.89 n=1295  OOS_PF=1.61 n=569 bp=+14813 dd=1175
chimera::EdgeEngine::Config s43b_pythusdt_tsmom_7200_45_5_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_tsmom_7200_45_5(s43b_pythusdt_tsmom_7200_45_5_cfg);
wire_engine(s43b_pythusdt_tsmom_7200_45_5);

// PYTH-TSMOM-H3  IS_PF=2.08 n=822  OOS_PF=1.65 n=405 bp=+12772 dd=1822
chimera::EdgeEngine::Config s43b_pythusdt_tsmom_10800_60_8_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_tsmom_10800_60_8(s43b_pythusdt_tsmom_10800_60_8_cfg);
wire_engine(s43b_pythusdt_tsmom_10800_60_8);

// PYTH-TSMOM-H4  IS_PF=2.66 n=639  OOS_PF=1.74 n=312 bp=+11594 dd=1920
chimera::EdgeEngine::Config s43b_pythusdt_tsmom_14400_45_8_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_tsmom_14400_45_8(s43b_pythusdt_tsmom_14400_45_8_cfg);
wire_engine(s43b_pythusdt_tsmom_14400_45_8);

// PYTH-TSMOM-H6  IS_PF=1.97 n=437  OOS_PF=1.90 n=210 bp=+12262 dd=2017
chimera::EdgeEngine::Config s43b_pythusdt_tsmom_21600_30_3_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_tsmom_21600_30_3(s43b_pythusdt_tsmom_21600_30_3_cfg);
wire_engine(s43b_pythusdt_tsmom_21600_30_3);

// PYTH-TSMOM-H8  IS_PF=2.32 n=327  OOS_PF=1.62 n=164 bp=+8627 dd=2107
chimera::EdgeEngine::Config s43b_pythusdt_tsmom_28800_18_5_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_pythusdt_tsmom_28800_18_5(s43b_pythusdt_tsmom_28800_18_5_cfg);
wire_engine(s43b_pythusdt_tsmom_28800_18_5);

// RENDER-ICHI-H1  IS_PF=1.67 n=976  OOS_PF=1.84 n=464 bp=+16020 dd=3570
chimera::EdgeEngine::Config s43b_renderusdt_ichimoku_3600_6_24_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_ichimoku_3600_6_24(s43b_renderusdt_ichimoku_3600_6_24_cfg);
wire_engine(s43b_renderusdt_ichimoku_3600_6_24);

// RENDER-ICHI-H12  IS_PF=1.51 n=99  OOS_PF=2.00 n=73 bp=+7535 dd=1868
chimera::EdgeEngine::Config s43b_renderusdt_ichimoku_43200_6_3_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_ichimoku_43200_6_3(s43b_renderusdt_ichimoku_43200_6_3_cfg);
wire_engine(s43b_renderusdt_ichimoku_43200_6_3);

// RENDER-ICHI-H2  IS_PF=1.56 n=549  OOS_PF=2.50 n=282 bp=+16349 dd=2020
chimera::EdgeEngine::Config s43b_renderusdt_ichimoku_7200_6_18_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_ichimoku_7200_6_18(s43b_renderusdt_ichimoku_7200_6_18_cfg);
wire_engine(s43b_renderusdt_ichimoku_7200_6_18);

// RENDER-ICHI-H3  IS_PF=2.38 n=379  OOS_PF=1.65 n=199 bp=+9494 dd=2758
chimera::EdgeEngine::Config s43b_renderusdt_ichimoku_10800_6_5_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_ichimoku_10800_6_5(s43b_renderusdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_renderusdt_ichimoku_10800_6_5);

// RENDER-ICHI-H4  IS_PF=1.69 n=329  OOS_PF=1.82 n=191 bp=+11843 dd=2012
chimera::EdgeEngine::Config s43b_renderusdt_ichimoku_14400_6_5_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_ichimoku_14400_6_5(s43b_renderusdt_ichimoku_14400_6_5_cfg);
wire_engine(s43b_renderusdt_ichimoku_14400_6_5);

// RENDER-ICHI-H6  IS_PF=1.99 n=227  OOS_PF=1.75 n=133 bp=+9800 dd=1953
chimera::EdgeEngine::Config s43b_renderusdt_ichimoku_21600_6_3_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_ichimoku_21600_6_3(s43b_renderusdt_ichimoku_21600_6_3_cfg);
wire_engine(s43b_renderusdt_ichimoku_21600_6_3);

// RENDER-TSMOM-D1  IS_PF=2.58 n=88  OOS_PF=1.73 n=54 bp=+6867 dd=2804
chimera::EdgeEngine::Config s43b_renderusdt_tsmom_86400_18_3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_tsmom_86400_18_3(s43b_renderusdt_tsmom_86400_18_3_cfg);
wire_engine(s43b_renderusdt_tsmom_86400_18_3);

// RENDER-TSMOM-H12  IS_PF=2.73 n=181  OOS_PF=2.48 n=142 bp=+17993 dd=1868
chimera::EdgeEngine::Config s43b_renderusdt_tsmom_43200_60_3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_tsmom_43200_60_3(s43b_renderusdt_tsmom_43200_60_3_cfg);
wire_engine(s43b_renderusdt_tsmom_43200_60_3);

// RENDER-TSMOM-H2  IS_PF=1.76 n=1272  OOS_PF=1.66 n=645 bp=+19896 dd=1471
chimera::EdgeEngine::Config s43b_renderusdt_tsmom_7200_60_3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_tsmom_7200_60_3(s43b_renderusdt_tsmom_7200_60_3_cfg);
wire_engine(s43b_renderusdt_tsmom_7200_60_3);

// RENDER-TSMOM-H3  IS_PF=1.93 n=843  OOS_PF=1.89 n=445 bp=+20537 dd=1752
chimera::EdgeEngine::Config s43b_renderusdt_tsmom_10800_60_8_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_tsmom_10800_60_8(s43b_renderusdt_tsmom_10800_60_8_cfg);
wire_engine(s43b_renderusdt_tsmom_10800_60_8);

// RENDER-TSMOM-H4  IS_PF=1.97 n=652  OOS_PF=1.58 n=345 bp=+13969 dd=1731
chimera::EdgeEngine::Config s43b_renderusdt_tsmom_14400_30_5_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_tsmom_14400_30_5(s43b_renderusdt_tsmom_14400_30_5_cfg);
wire_engine(s43b_renderusdt_tsmom_14400_30_5);

// RENDER-TSMOM-H6  IS_PF=2.08 n=440  OOS_PF=1.97 n=225 bp=+17415 dd=1953
chimera::EdgeEngine::Config s43b_renderusdt_tsmom_21600_30_3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_tsmom_21600_30_3(s43b_renderusdt_tsmom_21600_30_3_cfg);
wire_engine(s43b_renderusdt_tsmom_21600_30_3);

// RENDER-TSMOM-H8  IS_PF=2.78 n=317  OOS_PF=2.13 n=179 bp=+15771 dd=2467
chimera::EdgeEngine::Config s43b_renderusdt_tsmom_28800_18_5_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_renderusdt_tsmom_28800_18_5(s43b_renderusdt_tsmom_28800_18_5_cfg);
wire_engine(s43b_renderusdt_tsmom_28800_18_5);

// SEI-ICHI-H4  IS_PF=1.96 n=345  OOS_PF=1.77 n=104 bp=+4417 dd=1125
chimera::EdgeEngine::Config s43b_seiusdt_ichimoku_14400_6_5_cfg{
    .symbol="seiusdt", .tag="SEI-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_seiusdt_ichimoku_14400_6_5(s43b_seiusdt_ichimoku_14400_6_5_cfg);
wire_engine(s43b_seiusdt_ichimoku_14400_6_5);

// SEI-TSMOM-H4  IS_PF=2.37 n=605  OOS_PF=2.21 n=260 bp=+11455 dd=1275
chimera::EdgeEngine::Config s43b_seiusdt_tsmom_14400_60_5_cfg{
    .symbol="seiusdt", .tag="SEI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_seiusdt_tsmom_14400_60_5(s43b_seiusdt_tsmom_14400_60_5_cfg);
wire_engine(s43b_seiusdt_tsmom_14400_60_5);

// SEI-TSMOM-H6  IS_PF=2.79 n=406  OOS_PF=1.55 n=174 bp=+6088 dd=2480
chimera::EdgeEngine::Config s43b_seiusdt_tsmom_21600_45_3_cfg{
    .symbol="seiusdt", .tag="SEI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_seiusdt_tsmom_21600_45_3(s43b_seiusdt_tsmom_21600_45_3_cfg);
wire_engine(s43b_seiusdt_tsmom_21600_45_3);

// SEI-TSMOM-H8  IS_PF=3.12 n=294  OOS_PF=1.83 n=126 bp=+6234 dd=1455
chimera::EdgeEngine::Config s43b_seiusdt_tsmom_28800_60_3_cfg{
    .symbol="seiusdt", .tag="SEI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_seiusdt_tsmom_28800_60_3(s43b_seiusdt_tsmom_28800_60_3_cfg);
wire_engine(s43b_seiusdt_tsmom_28800_60_3);

// SUI-ICHI-H3  IS_PF=1.94 n=532  OOS_PF=2.36 n=196 bp=+10964 dd=2040
chimera::EdgeEngine::Config s43b_suiusdt_ichimoku_10800_6_5_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_ichimoku_10800_6_5(s43b_suiusdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_suiusdt_ichimoku_10800_6_5);

// SUI-ICHI-H4  IS_PF=1.77 n=340  OOS_PF=2.54 n=143 bp=+10895 dd=1166
chimera::EdgeEngine::Config s43b_suiusdt_ichimoku_14400_6_5_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_ichimoku_14400_6_5(s43b_suiusdt_ichimoku_14400_6_5_cfg);
wire_engine(s43b_suiusdt_ichimoku_14400_6_5);

// SUI-ICHI-H8  IS_PF=2.27 n=185  OOS_PF=2.42 n=55 bp=+5957 dd=1335
chimera::EdgeEngine::Config s43b_suiusdt_ichimoku_28800_6_8_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_ichimoku_28800_6_8(s43b_suiusdt_ichimoku_28800_6_8_cfg);
wire_engine(s43b_suiusdt_ichimoku_28800_6_8);

// SUI-TSMOM-H2  IS_PF=1.63 n=1386  OOS_PF=1.68 n=620 bp=+16108 dd=1319
chimera::EdgeEngine::Config s43b_suiusdt_tsmom_7200_30_5_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_tsmom_7200_30_5(s43b_suiusdt_tsmom_7200_30_5_cfg);
wire_engine(s43b_suiusdt_tsmom_7200_30_5);

// SUI-TSMOM-H3  IS_PF=1.85 n=848  OOS_PF=2.14 n=399 bp=+17431 dd=842
chimera::EdgeEngine::Config s43b_suiusdt_tsmom_10800_60_8_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_tsmom_10800_60_8(s43b_suiusdt_tsmom_10800_60_8_cfg);
wire_engine(s43b_suiusdt_tsmom_10800_60_8);

// SUI-TSMOM-H4  IS_PF=2.17 n=660  OOS_PF=2.49 n=313 bp=+18558 dd=1369
chimera::EdgeEngine::Config s43b_suiusdt_tsmom_14400_60_3_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_tsmom_14400_60_3(s43b_suiusdt_tsmom_14400_60_3_cfg);
wire_engine(s43b_suiusdt_tsmom_14400_60_3);

// SUI-TSMOM-H6  IS_PF=2.65 n=371  OOS_PF=1.89 n=206 bp=+10504 dd=2844
chimera::EdgeEngine::Config s43b_suiusdt_tsmom_21600_60_3_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_tsmom_21600_60_3(s43b_suiusdt_tsmom_21600_60_3_cfg);
wire_engine(s43b_suiusdt_tsmom_21600_60_3);

// SUI-TSMOM-H8  IS_PF=3.49 n=245  OOS_PF=1.88 n=131 bp=+10263 dd=2389
chimera::EdgeEngine::Config s43b_suiusdt_tsmom_28800_45_8_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_suiusdt_tsmom_28800_45_8(s43b_suiusdt_tsmom_28800_45_8_cfg);
wire_engine(s43b_suiusdt_tsmom_28800_45_8);

// TIA-ICHI-H2  IS_PF=1.94 n=621  OOS_PF=2.04 n=230 bp=+10817 dd=2260
chimera::EdgeEngine::Config s43b_tiausdt_ichimoku_7200_6_3_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_ichimoku_7200_6_3(s43b_tiausdt_ichimoku_7200_6_3_cfg);
wire_engine(s43b_tiausdt_ichimoku_7200_6_3);

// TIA-ICHI-H3  IS_PF=1.95 n=439  OOS_PF=1.64 n=186 bp=+6756 dd=2591
chimera::EdgeEngine::Config s43b_tiausdt_ichimoku_10800_6_5_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_ichimoku_10800_6_5(s43b_tiausdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_tiausdt_ichimoku_10800_6_5);

// TIA-ICHI-H4  IS_PF=2.03 n=272  OOS_PF=1.90 n=99 bp=+6504 dd=1896
chimera::EdgeEngine::Config s43b_tiausdt_ichimoku_14400_6_3_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_ichimoku_14400_6_3(s43b_tiausdt_ichimoku_14400_6_3_cfg);
wire_engine(s43b_tiausdt_ichimoku_14400_6_3);

// TIA-TSMOM-H12  IS_PF=2.88 n=129  OOS_PF=4.05 n=76 bp=+10310 dd=1281
chimera::EdgeEngine::Config s43b_tiausdt_tsmom_43200_60_5_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_tsmom_43200_60_5(s43b_tiausdt_tsmom_43200_60_5_cfg);
wire_engine(s43b_tiausdt_tsmom_43200_60_5);

// TIA-TSMOM-H2  IS_PF=2.02 n=1269  OOS_PF=1.53 n=611 bp=+14285 dd=1378
chimera::EdgeEngine::Config s43b_tiausdt_tsmom_7200_60_3_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_tsmom_7200_60_3(s43b_tiausdt_tsmom_7200_60_3_cfg);
wire_engine(s43b_tiausdt_tsmom_7200_60_3);

// TIA-TSMOM-H3  IS_PF=2.24 n=858  OOS_PF=1.57 n=401 bp=+13265 dd=2845
chimera::EdgeEngine::Config s43b_tiausdt_tsmom_10800_45_5_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_tsmom_10800_45_5(s43b_tiausdt_tsmom_10800_45_5_cfg);
wire_engine(s43b_tiausdt_tsmom_10800_45_5);

// TIA-TSMOM-H4  IS_PF=2.00 n=662  OOS_PF=2.16 n=321 bp=+20187 dd=1776
chimera::EdgeEngine::Config s43b_tiausdt_tsmom_14400_30_3_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_tsmom_14400_30_3(s43b_tiausdt_tsmom_14400_30_3_cfg);
wire_engine(s43b_tiausdt_tsmom_14400_30_3);

// TIA-TSMOM-H6  IS_PF=2.37 n=448  OOS_PF=1.74 n=221 bp=+13116 dd=2484
chimera::EdgeEngine::Config s43b_tiausdt_tsmom_21600_18_3_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tiausdt_tsmom_21600_18_3(s43b_tiausdt_tsmom_21600_18_3_cfg);
wire_engine(s43b_tiausdt_tsmom_21600_18_3);

// TON-ICHI-H2  IS_PF=1.73 n=424  OOS_PF=2.55 n=241 bp=+11317 dd=1894
chimera::EdgeEngine::Config s43b_tonusdt_ichimoku_7200_6_18_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tonusdt_ichimoku_7200_6_18(s43b_tonusdt_ichimoku_7200_6_18_cfg);
wire_engine(s43b_tonusdt_ichimoku_7200_6_18);

// TON-ICHI-H3  IS_PF=2.77 n=267  OOS_PF=2.10 n=156 bp=+7703 dd=1252
chimera::EdgeEngine::Config s43b_tonusdt_ichimoku_10800_6_18_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tonusdt_ichimoku_10800_6_18(s43b_tonusdt_ichimoku_10800_6_18_cfg);
wire_engine(s43b_tonusdt_ichimoku_10800_6_18);

// TON-ICHI-H6  IS_PF=4.93 n=125  OOS_PF=2.58 n=65 bp=+7698 dd=2928
chimera::EdgeEngine::Config s43b_tonusdt_ichimoku_21600_6_12_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tonusdt_ichimoku_21600_6_12(s43b_tonusdt_ichimoku_21600_6_12_cfg);
wire_engine(s43b_tonusdt_ichimoku_21600_6_12);

// TON-ICHI-H8  IS_PF=2.23 n=128  OOS_PF=2.65 n=62 bp=+7119 dd=2137
chimera::EdgeEngine::Config s43b_tonusdt_ichimoku_28800_6_8_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tonusdt_ichimoku_28800_6_8(s43b_tonusdt_ichimoku_28800_6_8_cfg);
wire_engine(s43b_tonusdt_ichimoku_28800_6_8);

// TON-TSMOM-H12  IS_PF=3.30 n=143  OOS_PF=2.13 n=76 bp=+8054 dd=2644
chimera::EdgeEngine::Config s43b_tonusdt_tsmom_43200_18_24_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tonusdt_tsmom_43200_18_24(s43b_tonusdt_tsmom_43200_18_24_cfg);
wire_engine(s43b_tonusdt_tsmom_43200_18_24);

// TON-TSMOM-H2  IS_PF=1.59 n=1132  OOS_PF=1.76 n=576 bp=+15185 dd=1614
chimera::EdgeEngine::Config s43b_tonusdt_tsmom_7200_60_18_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tonusdt_tsmom_7200_60_18(s43b_tonusdt_tsmom_7200_60_18_cfg);
wire_engine(s43b_tonusdt_tsmom_7200_60_18);

// TON-TSMOM-H3  IS_PF=2.47 n=497  OOS_PF=1.69 n=245 bp=+8248 dd=2597
chimera::EdgeEngine::Config s43b_tonusdt_tsmom_10800_45_24_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_tonusdt_tsmom_10800_45_24(s43b_tonusdt_tsmom_10800_45_24_cfg);
wire_engine(s43b_tonusdt_tsmom_10800_45_24);

// TURBO-ICHI-H4  IS_PF=1.99 n=249  OOS_PF=1.99 n=73 bp=+9002 dd=1687
chimera::EdgeEngine::Config s43b_turbousdt_ichimoku_14400_6_5_cfg{
    .symbol="turbousdt", .tag="TURBO-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_turbousdt_ichimoku_14400_6_5(s43b_turbousdt_ichimoku_14400_6_5_cfg);
wire_engine(s43b_turbousdt_ichimoku_14400_6_5);

// TURBO-TSMOM-H2  IS_PF=2.21 n=1299  OOS_PF=1.73 n=565 bp=+22095 dd=2677
chimera::EdgeEngine::Config s43b_turbousdt_tsmom_7200_45_5_cfg{
    .symbol="turbousdt", .tag="TURBO-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_turbousdt_tsmom_7200_45_5(s43b_turbousdt_tsmom_7200_45_5_cfg);
wire_engine(s43b_turbousdt_tsmom_7200_45_5);

// TURBO-TSMOM-H3  IS_PF=2.64 n=709  OOS_PF=1.71 n=322 bp=+19542 dd=3265
chimera::EdgeEngine::Config s43b_turbousdt_tsmom_10800_30_8_cfg{
    .symbol="turbousdt", .tag="TURBO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_turbousdt_tsmom_10800_30_8(s43b_turbousdt_tsmom_10800_30_8_cfg);
wire_engine(s43b_turbousdt_tsmom_10800_30_8);

// TURBO-TSMOM-H4  IS_PF=2.32 n=598  OOS_PF=1.78 n=264 bp=+20131 dd=3747
chimera::EdgeEngine::Config s43b_turbousdt_tsmom_14400_12_5_cfg{
    .symbol="turbousdt", .tag="TURBO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=12, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_turbousdt_tsmom_14400_12_5(s43b_turbousdt_tsmom_14400_12_5_cfg);
wire_engine(s43b_turbousdt_tsmom_14400_12_5);

// W-TSMOM-H6  IS_PF=1.99 n=425  OOS_PF=1.91 n=121 bp=+7566 dd=1952
chimera::EdgeEngine::Config s43b_wusdt_tsmom_21600_60_5_cfg{
    .symbol="wusdt", .tag="W-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_wusdt_tsmom_21600_60_5(s43b_wusdt_tsmom_21600_60_5_cfg);
wire_engine(s43b_wusdt_tsmom_21600_60_5);

// WIF-TSMOM-H2  IS_PF=1.98 n=1290  OOS_PF=1.55 n=544 bp=+15779 dd=3367
chimera::EdgeEngine::Config s43b_wifusdt_tsmom_7200_45_5_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_wifusdt_tsmom_7200_45_5(s43b_wifusdt_tsmom_7200_45_5_cfg);
wire_engine(s43b_wifusdt_tsmom_7200_45_5);

// WIF-TSMOM-H4  IS_PF=1.83 n=651  OOS_PF=1.68 n=291 bp=+12989 dd=3296
chimera::EdgeEngine::Config s43b_wifusdt_tsmom_14400_60_3_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_wifusdt_tsmom_14400_60_3(s43b_wifusdt_tsmom_14400_60_3_cfg);
wire_engine(s43b_wifusdt_tsmom_14400_60_3);

// WIF-TSMOM-H8  IS_PF=3.20 n=230  OOS_PF=1.90 n=120 bp=+10260 dd=3522
chimera::EdgeEngine::Config s43b_wifusdt_tsmom_28800_30_8_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_wifusdt_tsmom_28800_30_8(s43b_wifusdt_tsmom_28800_30_8_cfg);
wire_engine(s43b_wifusdt_tsmom_28800_30_8);

// ZRO-ICHI-H3  IS_PF=2.06 n=485  OOS_PF=1.58 n=266 bp=+10040 dd=3664
chimera::EdgeEngine::Config s43b_zrousdt_ichimoku_10800_6_5_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_ichimoku_10800_6_5(s43b_zrousdt_ichimoku_10800_6_5_cfg);
wire_engine(s43b_zrousdt_ichimoku_10800_6_5);

// ZRO-ICHI-H4  IS_PF=3.21 n=246  OOS_PF=2.53 n=141 bp=+12689 dd=4405
chimera::EdgeEngine::Config s43b_zrousdt_ichimoku_14400_6_12_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_ichimoku_14400_6_12(s43b_zrousdt_ichimoku_14400_6_12_cfg);
wire_engine(s43b_zrousdt_ichimoku_14400_6_12);

// ZRO-ICHI-H8  IS_PF=2.22 n=152  OOS_PF=2.02 n=74 bp=+6756 dd=1821
chimera::EdgeEngine::Config s43b_zrousdt_ichimoku_28800_6_8_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_ichimoku_28800_6_8(s43b_zrousdt_ichimoku_28800_6_8_cfg);
wire_engine(s43b_zrousdt_ichimoku_28800_6_8);

// ZRO-TSMOM-D1  IS_PF=1.81 n=76  OOS_PF=3.82 n=58 bp=+18667 dd=2890
chimera::EdgeEngine::Config s43b_zrousdt_tsmom_86400_60_5_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_tsmom_86400_60_5(s43b_zrousdt_tsmom_86400_60_5_cfg);
wire_engine(s43b_zrousdt_tsmom_86400_60_5);

// ZRO-TSMOM-H1  IS_PF=1.53 n=2607  OOS_PF=1.62 n=1299 bp=+31690 dd=2561
chimera::EdgeEngine::Config s43b_zrousdt_tsmom_3600_45_8_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_tsmom_3600_45_8(s43b_zrousdt_tsmom_3600_45_8_cfg);
wire_engine(s43b_zrousdt_tsmom_3600_45_8);

// ZRO-TSMOM-H12  IS_PF=2.96 n=149  OOS_PF=3.84 n=77 bp=+16526 dd=2411
chimera::EdgeEngine::Config s43b_zrousdt_tsmom_43200_60_24_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_tsmom_43200_60_24(s43b_zrousdt_tsmom_43200_60_24_cfg);
wire_engine(s43b_zrousdt_tsmom_43200_60_24);

// ZRO-TSMOM-H2  IS_PF=2.20 n=1099  OOS_PF=1.73 n=613 bp=+23523 dd=2430
chimera::EdgeEngine::Config s43b_zrousdt_tsmom_7200_60_3_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_tsmom_7200_60_3(s43b_zrousdt_tsmom_7200_60_3_cfg);
wire_engine(s43b_zrousdt_tsmom_7200_60_3);

// ZRO-TSMOM-H3  IS_PF=2.24 n=791  OOS_PF=1.95 n=519 bp=+26092 dd=4131
chimera::EdgeEngine::Config s43b_zrousdt_tsmom_10800_60_5_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_tsmom_10800_60_5(s43b_zrousdt_tsmom_10800_60_5_cfg);
wire_engine(s43b_zrousdt_tsmom_10800_60_5);

// ZRO-TSMOM-H4  IS_PF=2.70 n=608  OOS_PF=2.10 n=396 bp=+23944 dd=2108
chimera::EdgeEngine::Config s43b_zrousdt_tsmom_14400_60_12_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_tsmom_14400_60_12(s43b_zrousdt_tsmom_14400_60_12_cfg);
wire_engine(s43b_zrousdt_tsmom_14400_60_12);

// ZRO-TSMOM-H6  IS_PF=2.03 n=411  OOS_PF=1.62 n=243 bp=+13556 dd=4055
chimera::EdgeEngine::Config s43b_zrousdt_tsmom_21600_60_8_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43b_zrousdt_tsmom_21600_60_8(s43b_zrousdt_tsmom_21600_60_8_cfg);
wire_engine(s43b_zrousdt_tsmom_21600_60_8);
