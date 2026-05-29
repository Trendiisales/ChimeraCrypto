// S43-REPROMOTE — 137 engines from S41+S42 that passed TRUE held-out walk-forward.
// Generated 137 engines on 2026-05-29.
//
// Validation protocol:
//   IS  window: [-1460d, -1095d]  (365d)
//   OOS window: [-1095d,  -730d]  (365d)
//   BOTH windows entirely PRE-S42-discover-cutoff (-730d), so configs could
//   not have overfit either slice.
//   Gate: IS PF >= 1.3  AND  OOS PF >= 1.3  AND  IS_bp > 0  AND  OOS_bp > 0
//         AND IS_trades >= 20  AND  OOS_trades >= 20
//         AND not data-underfilled (IS != OOS records).
//
// Backup of pre-cull state: /Users/jo/Chimera_Baselines/pre_cull_20260529_162344
//
// ─── CONFIG + ENGINE + WIRE ──────────────────────────────────────────────
// FET-TSMOM-H3 minPF=2.35 score=162560
chimera::EdgeEngine::Config s43_fetusdt_tsmom_h3_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_fetusdt_tsmom_h3(s43_fetusdt_tsmom_h3_cfg);
wire_engine(s43_fetusdt_tsmom_h3);

// ICP-ICHI-H2 minPF=2.75 score=138224
chimera::EdgeEngine::Config s43_icpusdt_ichi_h2_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_ichi_h2(s43_icpusdt_ichi_h2_cfg);
wire_engine(s43_icpusdt_ichi_h2);

// FET-ICHI-H12 minPF=2.63 score=27471
chimera::EdgeEngine::Config s43_fetusdt_ichi_h12_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_fetusdt_ichi_h12(s43_fetusdt_ichi_h12_cfg);
// S44-CULL: wire_engine(s43_fetusdt_ichi_h12);

// MANA-ICHI-H12 minPF=2.00 score=34560
chimera::EdgeEngine::Config s43_manausdt_ichi_h12_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_manausdt_ichi_h12(s43_manausdt_ichi_h12_cfg);
// S44-CULL: wire_engine(s43_manausdt_ichi_h12);

// ICP-ICHI-H3 minPF=2.10 score=89626
chimera::EdgeEngine::Config s43_icpusdt_ichi_h3_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_ichi_h3(s43_icpusdt_ichi_h3_cfg);
wire_engine(s43_icpusdt_ichi_h3);

// LINK-ICHI-H12 minPF=2.17 score=43576
chimera::EdgeEngine::Config s43_linkusdt_ichi_h12_cfg{
    .symbol="linkusdt", .tag="LINK-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_linkusdt_ichi_h12(s43_linkusdt_ichi_h12_cfg);
// S44-CULL: wire_engine(s43_linkusdt_ichi_h12);

// IMX-TSMOM-H6 minPF=2.10 score=102095
chimera::EdgeEngine::Config s43_imxusdt_tsmom_h6_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_imxusdt_tsmom_h6(s43_imxusdt_tsmom_h6_cfg);
wire_engine(s43_imxusdt_tsmom_h6);

// LDO-ICHI-H12 minPF=2.18 score=41784
chimera::EdgeEngine::Config s43_ldousdt_ichi_h12_cfg{
    .symbol="ldousdt", .tag="LDO-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ldousdt_ichi_h12(s43_ldousdt_ichi_h12_cfg);
// S44-CULL: wire_engine(s43_ldousdt_ichi_h12);

// LDO-TSMOM-D2 minPF=2.32 score=54480
chimera::EdgeEngine::Config s43_ldousdt_tsmom_d2_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ldousdt_tsmom_d2(s43_ldousdt_tsmom_d2_cfg);
// S44-CULL: wire_engine(s43_ldousdt_tsmom_d2);

// CRV-ICHI-H12 minPF=2.18 score=55876
chimera::EdgeEngine::Config s43_crvusdt_ichi_h12_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_crvusdt_ichi_h12(s43_crvusdt_ichi_h12_cfg);
wire_engine(s43_crvusdt_ichi_h12);

// ICP-TSMOM-D2 minPF=2.16 score=63505
chimera::EdgeEngine::Config s43_icpusdt_tsmom_d2_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=6, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_d2(s43_icpusdt_tsmom_d2_cfg);
// S44-CULL: wire_engine(s43_icpusdt_tsmom_d2);

// AVAX-TSMOM-H12 minPF=2.00 score=59627
chimera::EdgeEngine::Config s43_avaxusdt_tsmom_h12_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_avaxusdt_tsmom_h12(s43_avaxusdt_tsmom_h12_cfg);
wire_engine(s43_avaxusdt_tsmom_h12);

// COMP-ICHI-H4 minPF=2.01 score=74311
chimera::EdgeEngine::Config s43_compusdt_ichi_h4_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_compusdt_ichi_h4(s43_compusdt_ichi_h4_cfg);
wire_engine(s43_compusdt_ichi_h4);

// MANA-ICHI-H4 minPF=2.20 score=61459
chimera::EdgeEngine::Config s43_manausdt_ichi_h4_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_manausdt_ichi_h4(s43_manausdt_ichi_h4_cfg);
wire_engine(s43_manausdt_ichi_h4);

// ICP-TSMOM-H3 minPF=2.11 score=143574
chimera::EdgeEngine::Config s43_icpusdt_tsmom_h3_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_h3(s43_icpusdt_tsmom_h3_cfg);
wire_engine(s43_icpusdt_tsmom_h3);

// INJ-ICHI-H8 minPF=2.26 score=63965
chimera::EdgeEngine::Config s43_injusdt_ichi_h8_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_injusdt_ichi_h8(s43_injusdt_ichi_h8_cfg);
wire_engine(s43_injusdt_ichi_h8);

// NEAR-ICHI-H3 minPF=2.04 score=73440
chimera::EdgeEngine::Config s43_nearusdt_ichi_h3_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_nearusdt_ichi_h3(s43_nearusdt_ichi_h3_cfg);
wire_engine(s43_nearusdt_ichi_h3);

// LDO-TSMOM-D1 minPF=2.15 score=67144
chimera::EdgeEngine::Config s43_ldousdt_tsmom_d1_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ldousdt_tsmom_d1(s43_ldousdt_tsmom_d1_cfg);
wire_engine(s43_ldousdt_tsmom_d1);

// COMP-ICHI-H6 minPF=3.31 score=80611
chimera::EdgeEngine::Config s43_compusdt_ichi_h6_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_compusdt_ichi_h6(s43_compusdt_ichi_h6_cfg);
wire_engine(s43_compusdt_ichi_h6);

// ATOM-ICHI-H3 minPF=2.06 score=59528
chimera::EdgeEngine::Config s43_atomusdt_ichi_h3_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_atomusdt_ichi_h3(s43_atomusdt_ichi_h3_cfg);
wire_engine(s43_atomusdt_ichi_h3);

// DOGE-TSMOM-D1 minPF=2.79 score=63684
chimera::EdgeEngine::Config s43_dogeusdt_tsmom_d1_cfg{
    .symbol="dogeusdt", .tag="DOGE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_dogeusdt_tsmom_d1(s43_dogeusdt_tsmom_d1_cfg);
// S44-CULL: wire_engine(s43_dogeusdt_tsmom_d1);

// MANA-ICHI-H6 minPF=2.40 score=56330
chimera::EdgeEngine::Config s43_manausdt_ichi_h6_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_manausdt_ichi_h6(s43_manausdt_ichi_h6_cfg);
wire_engine(s43_manausdt_ichi_h6);

// IMX-ICHI-H4 minPF=2.28 score=76119
chimera::EdgeEngine::Config s43_imxusdt_ichi_h4_cfg{
    .symbol="imxusdt", .tag="IMX-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_imxusdt_ichi_h4(s43_imxusdt_ichi_h4_cfg);
wire_engine(s43_imxusdt_ichi_h4);

// CRV-ICHI-H8 minPF=2.00 score=60083
chimera::EdgeEngine::Config s43_crvusdt_ichi_h8_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_crvusdt_ichi_h8(s43_crvusdt_ichi_h8_cfg);
wire_engine(s43_crvusdt_ichi_h8);

// MANA-TSMOM-H6 minPF=2.06 score=102244
chimera::EdgeEngine::Config s43_manausdt_tsmom_h6_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_manausdt_tsmom_h6(s43_manausdt_tsmom_h6_cfg);
wire_engine(s43_manausdt_tsmom_h6);

// ICP-TSMOM-D1 minPF=2.19 score=60065
chimera::EdgeEngine::Config s43_icpusdt_tsmom_d1_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_d1(s43_icpusdt_tsmom_d1_cfg);
wire_engine(s43_icpusdt_tsmom_d1);

// TRX-ICHI-D1 minPF=3.95 score=41492
chimera::EdgeEngine::Config s43_trxusdt_ichi_d1_cfg{
    .symbol="trxusdt", .tag="TRX-ICHI-D1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=86400, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_trxusdt_ichi_d1(s43_trxusdt_ichi_d1_cfg);
wire_engine(s43_trxusdt_ichi_d1);

// ICP-TSMOM-H2 minPF=2.00 score=148184
chimera::EdgeEngine::Config s43_icpusdt_tsmom_h2_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_h2(s43_icpusdt_tsmom_h2_cfg);
wire_engine(s43_icpusdt_tsmom_h2);

// JTO-TSMOM-H4 minPF=2.41 score=248121
chimera::EdgeEngine::Config s43_jtousdt_tsmom_h4_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jtousdt_tsmom_h4(s43_jtousdt_tsmom_h4_cfg);
wire_engine(s43_jtousdt_tsmom_h4);

// JTO-TSMOM-H3 minPF=2.25 score=232905
chimera::EdgeEngine::Config s43_jtousdt_tsmom_h3_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=18, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jtousdt_tsmom_h3(s43_jtousdt_tsmom_h3_cfg);
wire_engine(s43_jtousdt_tsmom_h3);

// JTO-TSMOM-H8 minPF=2.71 score=222662
chimera::EdgeEngine::Config s43_jtousdt_tsmom_h8_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jtousdt_tsmom_h8(s43_jtousdt_tsmom_h8_cfg);
wire_engine(s43_jtousdt_tsmom_h8);

// BOME-TSMOM-H4 minPF=2.10 score=217727
chimera::EdgeEngine::Config s43_bomeusdt_tsmom_h4_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_bomeusdt_tsmom_h4(s43_bomeusdt_tsmom_h4_cfg);
wire_engine(s43_bomeusdt_tsmom_h4);

// BOME-TSMOM-H3 minPF=2.00 score=211262
chimera::EdgeEngine::Config s43_bomeusdt_tsmom_h3_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_bomeusdt_tsmom_h3(s43_bomeusdt_tsmom_h3_cfg);
wire_engine(s43_bomeusdt_tsmom_h3);

// JTO-TSMOM-H12 minPF=2.46 score=208417
chimera::EdgeEngine::Config s43_jtousdt_tsmom_h12_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jtousdt_tsmom_h12(s43_jtousdt_tsmom_h12_cfg);
wire_engine(s43_jtousdt_tsmom_h12);

// INJ-TSMOM-H3 minPF=2.10 score=199704
chimera::EdgeEngine::Config s43_injusdt_tsmom_h3_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_injusdt_tsmom_h3(s43_injusdt_tsmom_h3_cfg);
wire_engine(s43_injusdt_tsmom_h3);

// JTO-TSMOM-H6 minPF=2.24 score=192076
chimera::EdgeEngine::Config s43_jtousdt_tsmom_h6_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jtousdt_tsmom_h6(s43_jtousdt_tsmom_h6_cfg);
// S44-CULL: wire_engine(s43_jtousdt_tsmom_h6);

// INJ-TSMOM-H4 minPF=2.25 score=185680
chimera::EdgeEngine::Config s43_injusdt_tsmom_h4_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_injusdt_tsmom_h4(s43_injusdt_tsmom_h4_cfg);
wire_engine(s43_injusdt_tsmom_h4);

// ENA-TSMOM-H3 minPF=2.16 score=183466
chimera::EdgeEngine::Config s43_enausdt_tsmom_h3_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_enausdt_tsmom_h3(s43_enausdt_tsmom_h3_cfg);
wire_engine(s43_enausdt_tsmom_h3);

// BOME-TSMOM-H6 minPF=2.16 score=172404
chimera::EdgeEngine::Config s43_bomeusdt_tsmom_h6_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_bomeusdt_tsmom_h6(s43_bomeusdt_tsmom_h6_cfg);
wire_engine(s43_bomeusdt_tsmom_h6);

// COMP-TSMOM-H3 minPF=2.07 score=170590
chimera::EdgeEngine::Config s43_compusdt_tsmom_h3_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_compusdt_tsmom_h3(s43_compusdt_tsmom_h3_cfg);
wire_engine(s43_compusdt_tsmom_h3);

// ICP-TSMOM-H4 minPF=2.41 score=167999
chimera::EdgeEngine::Config s43_icpusdt_tsmom_h4_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_h4(s43_icpusdt_tsmom_h4_cfg);
wire_engine(s43_icpusdt_tsmom_h4);

// TIA-TSMOM-H4 minPF=2.00 score=166613
chimera::EdgeEngine::Config s43_tiausdt_tsmom_h4_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_tiausdt_tsmom_h4(s43_tiausdt_tsmom_h4_cfg);
wire_engine(s43_tiausdt_tsmom_h4);

// WIF-TSMOM-H6 minPF=2.22 score=165662
chimera::EdgeEngine::Config s43_wifusdt_tsmom_h6_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_wifusdt_tsmom_h6(s43_wifusdt_tsmom_h6_cfg);
wire_engine(s43_wifusdt_tsmom_h6);

// COMP-TSMOM-H4 minPF=2.14 score=165463
chimera::EdgeEngine::Config s43_compusdt_tsmom_h4_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_compusdt_tsmom_h4(s43_compusdt_tsmom_h4_cfg);
wire_engine(s43_compusdt_tsmom_h4);

// FET-TSMOM-H6 minPF=2.90 score=164947
chimera::EdgeEngine::Config s43_fetusdt_tsmom_h6_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_fetusdt_tsmom_h6(s43_fetusdt_tsmom_h6_cfg);
wire_engine(s43_fetusdt_tsmom_h6);

// BOME-TSMOM-H8 minPF=2.26 score=162267
chimera::EdgeEngine::Config s43_bomeusdt_tsmom_h8_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_bomeusdt_tsmom_h8(s43_bomeusdt_tsmom_h8_cfg);
wire_engine(s43_bomeusdt_tsmom_h8);

// INJ-TSMOM-H6 minPF=2.42 score=159728
chimera::EdgeEngine::Config s43_injusdt_tsmom_h6_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_injusdt_tsmom_h6(s43_injusdt_tsmom_h6_cfg);
wire_engine(s43_injusdt_tsmom_h6);

// JUP-TSMOM-H4 minPF=2.01 score=158080
chimera::EdgeEngine::Config s43_jupusdt_tsmom_h4_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jupusdt_tsmom_h4(s43_jupusdt_tsmom_h4_cfg);
wire_engine(s43_jupusdt_tsmom_h4);

// UNI-TSMOM-H4 minPF=2.17 score=156889
chimera::EdgeEngine::Config s43_uniusdt_tsmom_h4_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_uniusdt_tsmom_h4(s43_uniusdt_tsmom_h4_cfg);
wire_engine(s43_uniusdt_tsmom_h4);

// IMX-TSMOM-H4 minPF=2.34 score=150505
chimera::EdgeEngine::Config s43_imxusdt_tsmom_h4_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_imxusdt_tsmom_h4(s43_imxusdt_tsmom_h4_cfg);
wire_engine(s43_imxusdt_tsmom_h4);

// TIA-TSMOM-H6 minPF=2.06 score=149745
chimera::EdgeEngine::Config s43_tiausdt_tsmom_h6_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_tiausdt_tsmom_h6(s43_tiausdt_tsmom_h6_cfg);
wire_engine(s43_tiausdt_tsmom_h6);

// COMP-TSMOM-H6 minPF=2.35 score=148181
chimera::EdgeEngine::Config s43_compusdt_tsmom_h6_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_compusdt_tsmom_h6(s43_compusdt_tsmom_h6_cfg);
wire_engine(s43_compusdt_tsmom_h6);

// FET-TSMOM-H4 minPF=2.29 score=146028
chimera::EdgeEngine::Config s43_fetusdt_tsmom_h4_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_fetusdt_tsmom_h4(s43_fetusdt_tsmom_h4_cfg);
wire_engine(s43_fetusdt_tsmom_h4);

// ICP-TSMOM-H8 minPF=2.43 score=145083
chimera::EdgeEngine::Config s43_icpusdt_tsmom_h8_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_h8(s43_icpusdt_tsmom_h8_cfg);
wire_engine(s43_icpusdt_tsmom_h8);

// ICP-TSMOM-H6 minPF=2.20 score=142219
chimera::EdgeEngine::Config s43_icpusdt_tsmom_h6_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_h6(s43_icpusdt_tsmom_h6_cfg);
wire_engine(s43_icpusdt_tsmom_h6);

// UNI-TSMOM-H6 minPF=2.45 score=141274
chimera::EdgeEngine::Config s43_uniusdt_tsmom_h6_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_uniusdt_tsmom_h6(s43_uniusdt_tsmom_h6_cfg);
wire_engine(s43_uniusdt_tsmom_h6);

// TIA-TSMOM-H3 minPF=2.15 score=139378
chimera::EdgeEngine::Config s43_tiausdt_tsmom_h3_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_tiausdt_tsmom_h3(s43_tiausdt_tsmom_h3_cfg);
wire_engine(s43_tiausdt_tsmom_h3);

// OP-TSMOM-H2 minPF=2.04 score=132799
chimera::EdgeEngine::Config s43_opusdt_tsmom_h2_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_opusdt_tsmom_h2(s43_opusdt_tsmom_h2_cfg);
wire_engine(s43_opusdt_tsmom_h2);

// IMX-TSMOM-H8 minPF=2.61 score=132754
chimera::EdgeEngine::Config s43_imxusdt_tsmom_h8_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_imxusdt_tsmom_h8(s43_imxusdt_tsmom_h8_cfg);
wire_engine(s43_imxusdt_tsmom_h8);

// LDO-TSMOM-H12 minPF=2.72 score=132114
chimera::EdgeEngine::Config s43_ldousdt_tsmom_h12_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ldousdt_tsmom_h12(s43_ldousdt_tsmom_h12_cfg);
wire_engine(s43_ldousdt_tsmom_h12);

// COMP-TSMOM-H8 minPF=2.43 score=130180
chimera::EdgeEngine::Config s43_compusdt_tsmom_h8_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_compusdt_tsmom_h8(s43_compusdt_tsmom_h8_cfg);
wire_engine(s43_compusdt_tsmom_h8);

// ENA-ICHI-H2 minPF=2.00 score=128237
chimera::EdgeEngine::Config s43_enausdt_ichi_h2_cfg{
    .symbol="enausdt", .tag="ENA-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_enausdt_ichi_h2(s43_enausdt_ichi_h2_cfg);
wire_engine(s43_enausdt_ichi_h2);

// UNI-TSMOM-H12 minPF=2.72 score=127728
chimera::EdgeEngine::Config s43_uniusdt_tsmom_h12_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_uniusdt_tsmom_h12(s43_uniusdt_tsmom_h12_cfg);
wire_engine(s43_uniusdt_tsmom_h12);

// STX-TSMOM-H6 minPF=2.05 score=126479
chimera::EdgeEngine::Config s43_stxusdt_tsmom_h6_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_stxusdt_tsmom_h6(s43_stxusdt_tsmom_h6_cfg);
wire_engine(s43_stxusdt_tsmom_h6);

// LDO-TSMOM-H8 minPF=2.15 score=123471
chimera::EdgeEngine::Config s43_ldousdt_tsmom_h8_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ldousdt_tsmom_h8(s43_ldousdt_tsmom_h8_cfg);
wire_engine(s43_ldousdt_tsmom_h8);

// UNI-TSMOM-H8 minPF=2.30 score=122060
chimera::EdgeEngine::Config s43_uniusdt_tsmom_h8_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_uniusdt_tsmom_h8(s43_uniusdt_tsmom_h8_cfg);
wire_engine(s43_uniusdt_tsmom_h8);

// ARKM-TSMOM-H12 minPF=2.25 score=120386
chimera::EdgeEngine::Config s43_arkmusdt_tsmom_h12_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_arkmusdt_tsmom_h12(s43_arkmusdt_tsmom_h12_cfg);
wire_engine(s43_arkmusdt_tsmom_h12);

// STX-TSMOM-H8 minPF=2.44 score=119831
chimera::EdgeEngine::Config s43_stxusdt_tsmom_h8_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_stxusdt_tsmom_h8(s43_stxusdt_tsmom_h8_cfg);
wire_engine(s43_stxusdt_tsmom_h8);

// JUP-TSMOM-H12 minPF=2.35 score=118092
chimera::EdgeEngine::Config s43_jupusdt_tsmom_h12_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jupusdt_tsmom_h12(s43_jupusdt_tsmom_h12_cfg);
wire_engine(s43_jupusdt_tsmom_h12);

// AVAX-TSMOM-H3 minPF=2.00 score=115188
chimera::EdgeEngine::Config s43_avaxusdt_tsmom_h3_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_avaxusdt_tsmom_h3(s43_avaxusdt_tsmom_h3_cfg);
wire_engine(s43_avaxusdt_tsmom_h3);

// COMP-TSMOM-H12 minPF=2.10 score=111111
chimera::EdgeEngine::Config s43_compusdt_tsmom_h12_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_compusdt_tsmom_h12(s43_compusdt_tsmom_h12_cfg);
wire_engine(s43_compusdt_tsmom_h12);

// AAVE-TSMOM-H8 minPF=2.16 score=110155
chimera::EdgeEngine::Config s43_aaveusdt_tsmom_h8_cfg{
    .symbol="aaveusdt", .tag="AAVE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_aaveusdt_tsmom_h8(s43_aaveusdt_tsmom_h8_cfg);
wire_engine(s43_aaveusdt_tsmom_h8);

// CRV-TSMOM-H12 minPF=2.65 score=109896
chimera::EdgeEngine::Config s43_crvusdt_tsmom_h12_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_crvusdt_tsmom_h12(s43_crvusdt_tsmom_h12_cfg);
wire_engine(s43_crvusdt_tsmom_h12);

// JUP-TSMOM-H8 minPF=2.01 score=107736
chimera::EdgeEngine::Config s43_jupusdt_tsmom_h8_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jupusdt_tsmom_h8(s43_jupusdt_tsmom_h8_cfg);
wire_engine(s43_jupusdt_tsmom_h8);

// IMX-TSMOM-H12 minPF=2.70 score=103267
chimera::EdgeEngine::Config s43_imxusdt_tsmom_h12_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_imxusdt_tsmom_h12(s43_imxusdt_tsmom_h12_cfg);
wire_engine(s43_imxusdt_tsmom_h12);

// MANA-TSMOM-H4 minPF=2.01 score=102167
chimera::EdgeEngine::Config s43_manausdt_tsmom_h4_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_manausdt_tsmom_h4(s43_manausdt_tsmom_h4_cfg);
wire_engine(s43_manausdt_tsmom_h4);

// STX-TSMOM-H12 minPF=2.13 score=100620
chimera::EdgeEngine::Config s43_stxusdt_tsmom_h12_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_stxusdt_tsmom_h12(s43_stxusdt_tsmom_h12_cfg);
wire_engine(s43_stxusdt_tsmom_h12);

// XLM-TSMOM-H4 minPF=2.01 score=99926
chimera::EdgeEngine::Config s43_xlmusdt_tsmom_h4_cfg{
    .symbol="xlmusdt", .tag="XLM-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_xlmusdt_tsmom_h4(s43_xlmusdt_tsmom_h4_cfg);
wire_engine(s43_xlmusdt_tsmom_h4);

// FET-ICHI-H3 minPF=2.02 score=99561
chimera::EdgeEngine::Config s43_fetusdt_ichi_h3_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_fetusdt_ichi_h3(s43_fetusdt_ichi_h3_cfg);
wire_engine(s43_fetusdt_ichi_h3);

// MASK-TSMOM-H12 minPF=2.27 score=98631
chimera::EdgeEngine::Config s43_maskusdt_tsmom_h12_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_maskusdt_tsmom_h12(s43_maskusdt_tsmom_h12_cfg);
wire_engine(s43_maskusdt_tsmom_h12);

// RUNE-TSMOM-H4 minPF=2.02 score=98210
chimera::EdgeEngine::Config s43_runeusdt_tsmom_h4_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_runeusdt_tsmom_h4(s43_runeusdt_tsmom_h4_cfg);
wire_engine(s43_runeusdt_tsmom_h4);

// SAND-TSMOM-H6 minPF=2.12 score=97606
chimera::EdgeEngine::Config s43_sandusdt_tsmom_h6_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_sandusdt_tsmom_h6(s43_sandusdt_tsmom_h6_cfg);
wire_engine(s43_sandusdt_tsmom_h6);

// ATOM-TSMOM-H8 minPF=2.78 score=97496
chimera::EdgeEngine::Config s43_atomusdt_tsmom_h8_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_atomusdt_tsmom_h8(s43_atomusdt_tsmom_h8_cfg);
wire_engine(s43_atomusdt_tsmom_h8);

// ICP-TSMOM-H12 minPF=2.15 score=96377
chimera::EdgeEngine::Config s43_icpusdt_tsmom_h12_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_tsmom_h12(s43_icpusdt_tsmom_h12_cfg);
wire_engine(s43_icpusdt_tsmom_h12);

// ARKM-TSMOM-H8 minPF=2.15 score=95482
chimera::EdgeEngine::Config s43_arkmusdt_tsmom_h8_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_arkmusdt_tsmom_h8(s43_arkmusdt_tsmom_h8_cfg);
wire_engine(s43_arkmusdt_tsmom_h8);

// DOT-TSMOM-H3 minPF=2.04 score=94304
chimera::EdgeEngine::Config s43_dotusdt_tsmom_h3_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_dotusdt_tsmom_h3(s43_dotusdt_tsmom_h3_cfg);
wire_engine(s43_dotusdt_tsmom_h3);

// ADA-TSMOM-H6 minPF=2.17 score=93543
chimera::EdgeEngine::Config s43_adausdt_tsmom_h6_cfg{
    .symbol="adausdt", .tag="ADA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_adausdt_tsmom_h6(s43_adausdt_tsmom_h6_cfg);
wire_engine(s43_adausdt_tsmom_h6);

// GMT-TSMOM-H12 minPF=2.13 score=92871
chimera::EdgeEngine::Config s43_gmtusdt_tsmom_h12_cfg{
    .symbol="gmtusdt", .tag="GMT-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_gmtusdt_tsmom_h12(s43_gmtusdt_tsmom_h12_cfg);
wire_engine(s43_gmtusdt_tsmom_h12);

// SAND-TSMOM-H8 minPF=2.10 score=92454
chimera::EdgeEngine::Config s43_sandusdt_tsmom_h8_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_sandusdt_tsmom_h8(s43_sandusdt_tsmom_h8_cfg);
wire_engine(s43_sandusdt_tsmom_h8);

// UNI-TSMOM-D1 minPF=2.21 score=91056
chimera::EdgeEngine::Config s43_uniusdt_tsmom_d1_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_uniusdt_tsmom_d1(s43_uniusdt_tsmom_d1_cfg);
wire_engine(s43_uniusdt_tsmom_d1);

// SOL-TSMOM-H8 minPF=2.75 score=90469
chimera::EdgeEngine::Config s43_solusdt_tsmom_h8_cfg{
    .symbol="solusdt", .tag="SOL-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_solusdt_tsmom_h8(s43_solusdt_tsmom_h8_cfg);
wire_engine(s43_solusdt_tsmom_h8);

// FET-ICHI-H4 minPF=2.55 score=89113
chimera::EdgeEngine::Config s43_fetusdt_ichi_h4_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=45, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_fetusdt_ichi_h4(s43_fetusdt_ichi_h4_cfg);
wire_engine(s43_fetusdt_ichi_h4);

// DOT-TSMOM-H6 minPF=2.17 score=87438
chimera::EdgeEngine::Config s43_dotusdt_tsmom_h6_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_dotusdt_tsmom_h6(s43_dotusdt_tsmom_h6_cfg);
wire_engine(s43_dotusdt_tsmom_h6);

// TIA-ICHI-H3 minPF=2.09 score=87112
chimera::EdgeEngine::Config s43_tiausdt_ichi_h3_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_tiausdt_ichi_h3(s43_tiausdt_ichi_h3_cfg);
wire_engine(s43_tiausdt_ichi_h3);

// XLM-TSMOM-H8 minPF=2.46 score=86993
chimera::EdgeEngine::Config s43_xlmusdt_tsmom_h8_cfg{
    .symbol="xlmusdt", .tag="XLM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_xlmusdt_tsmom_h8(s43_xlmusdt_tsmom_h8_cfg);
wire_engine(s43_xlmusdt_tsmom_h8);

// INJ-ICHI-H4 minPF=2.10 score=86682
chimera::EdgeEngine::Config s43_injusdt_ichi_h4_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_injusdt_ichi_h4(s43_injusdt_ichi_h4_cfg);
wire_engine(s43_injusdt_ichi_h4);

// IMX-TSMOM-D1 minPF=2.25 score=85850
chimera::EdgeEngine::Config s43_imxusdt_tsmom_d1_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_imxusdt_tsmom_d1(s43_imxusdt_tsmom_d1_cfg);
wire_engine(s43_imxusdt_tsmom_d1);

// ATOM-TSMOM-H6 minPF=2.00 score=83702
chimera::EdgeEngine::Config s43_atomusdt_tsmom_h6_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_atomusdt_tsmom_h6(s43_atomusdt_tsmom_h6_cfg);
wire_engine(s43_atomusdt_tsmom_h6);

// RUNE-TSMOM-H8 minPF=2.16 score=82517
chimera::EdgeEngine::Config s43_runeusdt_tsmom_h8_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_runeusdt_tsmom_h8(s43_runeusdt_tsmom_h8_cfg);
wire_engine(s43_runeusdt_tsmom_h8);

// MANA-TSMOM-H12 minPF=2.03 score=82467
chimera::EdgeEngine::Config s43_manausdt_tsmom_h12_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_manausdt_tsmom_h12(s43_manausdt_tsmom_h12_cfg);
wire_engine(s43_manausdt_tsmom_h12);

// ATOM-TSMOM-H12 minPF=2.84 score=79333
chimera::EdgeEngine::Config s43_atomusdt_tsmom_h12_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_atomusdt_tsmom_h12(s43_atomusdt_tsmom_h12_cfg);
wire_engine(s43_atomusdt_tsmom_h12);

// HBAR-TSMOM-H4 minPF=2.01 score=79081
chimera::EdgeEngine::Config s43_hbarusdt_tsmom_h4_cfg{
    .symbol="hbarusdt", .tag="HBAR-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_hbarusdt_tsmom_h4(s43_hbarusdt_tsmom_h4_cfg);
wire_engine(s43_hbarusdt_tsmom_h4);

// RUNE-TSMOM-H12 minPF=2.67 score=78965
chimera::EdgeEngine::Config s43_runeusdt_tsmom_h12_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_runeusdt_tsmom_h12(s43_runeusdt_tsmom_h12_cfg);
wire_engine(s43_runeusdt_tsmom_h12);

// DOT-TSMOM-H12 minPF=2.13 score=78263
chimera::EdgeEngine::Config s43_dotusdt_tsmom_h12_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_dotusdt_tsmom_h12(s43_dotusdt_tsmom_h12_cfg);
wire_engine(s43_dotusdt_tsmom_h12);

// ICP-ICHI-H4 minPF=2.03 score=78137
chimera::EdgeEngine::Config s43_icpusdt_ichi_h4_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=12, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_icpusdt_ichi_h4(s43_icpusdt_ichi_h4_cfg);
wire_engine(s43_icpusdt_ichi_h4);

// MANA-TSMOM-H8 minPF=2.00 score=78086
chimera::EdgeEngine::Config s43_manausdt_tsmom_h8_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_manausdt_tsmom_h8(s43_manausdt_tsmom_h8_cfg);
wire_engine(s43_manausdt_tsmom_h8);

// FIL-TSMOM-D1 minPF=2.36 score=77606
chimera::EdgeEngine::Config s43_filusdt_tsmom_d1_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_filusdt_tsmom_d1(s43_filusdt_tsmom_d1_cfg);
wire_engine(s43_filusdt_tsmom_d1);

// INJ-ICHI-H6 minPF=2.01 score=76791
chimera::EdgeEngine::Config s43_injusdt_ichi_h6_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_injusdt_ichi_h6(s43_injusdt_ichi_h6_cfg);
wire_engine(s43_injusdt_ichi_h6);

// XLM-TSMOM-H6 minPF=2.07 score=76682
chimera::EdgeEngine::Config s43_xlmusdt_tsmom_h6_cfg{
    .symbol="xlmusdt", .tag="XLM-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_xlmusdt_tsmom_h6(s43_xlmusdt_tsmom_h6_cfg);
wire_engine(s43_xlmusdt_tsmom_h6);

// MASK-TSMOM-D1 minPF=2.34 score=76606
chimera::EdgeEngine::Config s43_maskusdt_tsmom_d1_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_maskusdt_tsmom_d1(s43_maskusdt_tsmom_d1_cfg);
// S44-CULL: wire_engine(s43_maskusdt_tsmom_d1);

// ETH-TSMOM-H12 minPF=2.50 score=72332
chimera::EdgeEngine::Config s43_ethusdt_tsmom_h12_cfg{
    .symbol="ethusdt", .tag="ETH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=17, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ethusdt_tsmom_h12(s43_ethusdt_tsmom_h12_cfg);
wire_engine(s43_ethusdt_tsmom_h12);

// MASK-ICHI-H3 minPF=2.11 score=72103
chimera::EdgeEngine::Config s43_maskusdt_ichi_h3_cfg{
    .symbol="maskusdt", .tag="MASK-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_maskusdt_ichi_h3(s43_maskusdt_ichi_h3_cfg);
wire_engine(s43_maskusdt_ichi_h3);

// XLM-ICHI-H4 minPF=2.54 score=67644
chimera::EdgeEngine::Config s43_xlmusdt_ichi_h4_cfg{
    .symbol="xlmusdt", .tag="XLM-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_xlmusdt_ichi_h4(s43_xlmusdt_ichi_h4_cfg);
wire_engine(s43_xlmusdt_ichi_h4);

// LTC-TSMOM-H12 minPF=2.47 score=67368
chimera::EdgeEngine::Config s43_ltcusdt_tsmom_h12_cfg{
    .symbol="ltcusdt", .tag="LTC-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ltcusdt_tsmom_h12(s43_ltcusdt_tsmom_h12_cfg);
wire_engine(s43_ltcusdt_tsmom_h12);

// STX-TSMOM-D1 minPF=2.06 score=61571
chimera::EdgeEngine::Config s43_stxusdt_tsmom_d1_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_stxusdt_tsmom_d1(s43_stxusdt_tsmom_d1_cfg);
wire_engine(s43_stxusdt_tsmom_d1);

// CRV-TSMOM-D1 minPF=2.35 score=61102
chimera::EdgeEngine::Config s43_crvusdt_tsmom_d1_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_crvusdt_tsmom_d1(s43_crvusdt_tsmom_d1_cfg);
wire_engine(s43_crvusdt_tsmom_d1);

// BCH-TSMOM-D1 minPF=2.74 score=60400
chimera::EdgeEngine::Config s43_bchusdt_tsmom_d1_cfg{
    .symbol="bchusdt", .tag="BCH-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_bchusdt_tsmom_d1(s43_bchusdt_tsmom_d1_cfg);
wire_engine(s43_bchusdt_tsmom_d1);

// AVAX-TSMOM-D1 minPF=2.42 score=59618
chimera::EdgeEngine::Config s43_avaxusdt_tsmom_d1_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_avaxusdt_tsmom_d1(s43_avaxusdt_tsmom_d1_cfg);
// S44-CULL: wire_engine(s43_avaxusdt_tsmom_d1);

// JTO-ICHI-H8 minPF=2.06 score=57854
chimera::EdgeEngine::Config s43_jtousdt_ichi_h8_cfg{
    .symbol="jtousdt", .tag="JTO-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=12, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_jtousdt_ichi_h8(s43_jtousdt_ichi_h8_cfg);
wire_engine(s43_jtousdt_ichi_h8);

// SHIB-TSMOM-H8 minPF=2.01 score=56146
chimera::EdgeEngine::Config s43_shibusdt_tsmom_h8_cfg{
    .symbol="shibusdt", .tag="SHIB-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_shibusdt_tsmom_h8(s43_shibusdt_tsmom_h8_cfg);
wire_engine(s43_shibusdt_tsmom_h8);

// SHIB-TSMOM-H12 minPF=2.14 score=56061
chimera::EdgeEngine::Config s43_shibusdt_tsmom_h12_cfg{
    .symbol="shibusdt", .tag="SHIB-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_shibusdt_tsmom_h12(s43_shibusdt_tsmom_h12_cfg);
wire_engine(s43_shibusdt_tsmom_h12);

// RUNE-TSMOM-D1 minPF=2.12 score=56056
chimera::EdgeEngine::Config s43_runeusdt_tsmom_d1_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_runeusdt_tsmom_d1(s43_runeusdt_tsmom_d1_cfg);
wire_engine(s43_runeusdt_tsmom_d1);

// ATOM-ICHI-H4 minPF=2.25 score=55736
chimera::EdgeEngine::Config s43_atomusdt_ichi_h4_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_atomusdt_ichi_h4(s43_atomusdt_ichi_h4_cfg);
wire_engine(s43_atomusdt_ichi_h4);

// ATOM-TSMOM-D2 minPF=4.28 score=53014
chimera::EdgeEngine::Config s43_atomusdt_tsmom_d2_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_atomusdt_tsmom_d2(s43_atomusdt_tsmom_d2_cfg);
// S44-CULL: wire_engine(s43_atomusdt_tsmom_d2);

// BCH-TSMOM-H12 minPF=2.00 score=52867
chimera::EdgeEngine::Config s43_bchusdt_tsmom_h12_cfg{
    .symbol="bchusdt", .tag="BCH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_bchusdt_tsmom_h12(s43_bchusdt_tsmom_h12_cfg);
wire_engine(s43_bchusdt_tsmom_h12);

// DOGE-ICHI-H8 minPF=2.26 score=51792
chimera::EdgeEngine::Config s43_dogeusdt_ichi_h8_cfg{
    .symbol="dogeusdt", .tag="DOGE-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_dogeusdt_ichi_h8(s43_dogeusdt_ichi_h8_cfg);
// S44-CULL: wire_engine(s43_dogeusdt_ichi_h8);

// RUNE-ICHI-H8 minPF=2.77 score=50389
chimera::EdgeEngine::Config s43_runeusdt_ichi_h8_cfg{
    .symbol="runeusdt", .tag="RUNE-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=18, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_runeusdt_ichi_h8(s43_runeusdt_ichi_h8_cfg);
wire_engine(s43_runeusdt_ichi_h8);

// SOL-ICHI-H6 minPF=2.10 score=49336
chimera::EdgeEngine::Config s43_solusdt_ichi_h6_cfg{
    .symbol="solusdt", .tag="SOL-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_solusdt_ichi_h6(s43_solusdt_ichi_h6_cfg);
wire_engine(s43_solusdt_ichi_h6);

// ETC-TSMOM-D1 minPF=2.26 score=47033
chimera::EdgeEngine::Config s43_etcusdt_tsmom_d1_cfg{
    .symbol="etcusdt", .tag="ETC-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_etcusdt_tsmom_d1(s43_etcusdt_tsmom_d1_cfg);
// S44-CULL: wire_engine(s43_etcusdt_tsmom_d1);

// RUNE-TSMOM-D2 minPF=2.08 score=46973
chimera::EdgeEngine::Config s43_runeusdt_tsmom_d2_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_runeusdt_tsmom_d2(s43_runeusdt_tsmom_d2_cfg);
// S44-CULL: wire_engine(s43_runeusdt_tsmom_d2);

// ATOM-TSMOM-D1 minPF=2.22 score=46930
chimera::EdgeEngine::Config s43_atomusdt_tsmom_d1_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_atomusdt_tsmom_d1(s43_atomusdt_tsmom_d1_cfg);
wire_engine(s43_atomusdt_tsmom_d1);

// TRX-TSMOM-H12 minPF=2.08 score=46111
chimera::EdgeEngine::Config s43_trxusdt_tsmom_h12_cfg{
    .symbol="trxusdt", .tag="TRX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_trxusdt_tsmom_h12(s43_trxusdt_tsmom_h12_cfg);
wire_engine(s43_trxusdt_tsmom_h12);

// VET-TSMOM-D1 minPF=2.10 score=44104
chimera::EdgeEngine::Config s43_vetusdt_tsmom_d1_cfg{
    .symbol="vetusdt", .tag="VET-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_vetusdt_tsmom_d1(s43_vetusdt_tsmom_d1_cfg);
wire_engine(s43_vetusdt_tsmom_d1);

// LTC-TSMOM-D1 minPF=2.25 score=42190
chimera::EdgeEngine::Config s43_ltcusdt_tsmom_d1_cfg{
    .symbol="ltcusdt", .tag="LTC-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ltcusdt_tsmom_d1(s43_ltcusdt_tsmom_d1_cfg);
wire_engine(s43_ltcusdt_tsmom_d1);

// TRX-TSMOM-D3 minPF=2.87 score=38996
chimera::EdgeEngine::Config s43_trxusdt_tsmom_d3_cfg{
    .symbol="trxusdt", .tag="TRX-TSMOM-D3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=259200, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_trxusdt_tsmom_d3(s43_trxusdt_tsmom_d3_cfg);
// S44-CULL: wire_engine(s43_trxusdt_tsmom_d3);

// LTC-ICHI-H12 minPF=3.11 score=38706
chimera::EdgeEngine::Config s43_ltcusdt_ichi_h12_cfg{
    .symbol="ltcusdt", .tag="LTC-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_ltcusdt_ichi_h12(s43_ltcusdt_ichi_h12_cfg);
// S44-CULL: wire_engine(s43_ltcusdt_ichi_h12);

// RUNE-ICHI-H6 minPF=2.06 score=35350
chimera::EdgeEngine::Config s43_runeusdt_ichi_h6_cfg{
    .symbol="runeusdt", .tag="RUNE-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s43_runeusdt_ichi_h6(s43_runeusdt_ichi_h6_cfg);
wire_engine(s43_runeusdt_ichi_h6);
