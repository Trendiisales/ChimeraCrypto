// S41 CONSOLIDATED — 65 high-conviction engines (PF>=2.0 across all 4 WF windows)
// Replaces S38+S39+S40. MATIC/MKR delisted dropped. RNDR remapped to RENDER.
// Source: best of /tmp/wf_survivors + uwide + gx, dedup by (sym,tf,kind) keeping highest min PF.
// ─── CONFIG + ENGINE + WIRE ─────────────────────────────────────────────
// FET-TSMOM-H3 minPF=2.35 score=162560
chimera::EdgeEngine::Config s41_fetusdt_tsmom_h3_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_fetusdt_tsmom_h3(s41_fetusdt_tsmom_h3_cfg);
wire_engine(s41_fetusdt_tsmom_h3);

// ICP-ICHI-H2 minPF=2.75 score=138224
chimera::EdgeEngine::Config s41_icpusdt_ichi_h2_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_icpusdt_ichi_h2(s41_icpusdt_ichi_h2_cfg);
wire_engine(s41_icpusdt_ichi_h2);

// SEI-BOLL-H2 minPF=2.23 score=11582
chimera::EdgeEngine::Config s41_seiusdt_boll_h2_cfg{
    .symbol="seiusdt", .tag="SEI-BOLL-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=6, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_seiusdt_boll_h2(s41_seiusdt_boll_h2_cfg);
wire_engine(s41_seiusdt_boll_h2);

// FET-ICHI-H12 minPF=2.63 score=27471
chimera::EdgeEngine::Config s41_fetusdt_ichi_h12_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_fetusdt_ichi_h12(s41_fetusdt_ichi_h12_cfg);
wire_engine(s41_fetusdt_ichi_h12);

// MANA-ICHI-H12 minPF=2.00 score=34560
chimera::EdgeEngine::Config s41_manausdt_ichi_h12_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_manausdt_ichi_h12(s41_manausdt_ichi_h12_cfg);
wire_engine(s41_manausdt_ichi_h12);

// COMP-TSMOM-D1 minPF=2.23 score=63746
chimera::EdgeEngine::Config s41_compusdt_tsmom_d1_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_compusdt_tsmom_d1(s41_compusdt_tsmom_d1_cfg);
wire_engine(s41_compusdt_tsmom_d1);

// ICP-ICHI-H3 minPF=2.10 score=89626
chimera::EdgeEngine::Config s41_icpusdt_ichi_h3_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_icpusdt_ichi_h3(s41_icpusdt_ichi_h3_cfg);
wire_engine(s41_icpusdt_ichi_h3);

// ONDO-TSMOM-H8 minPF=2.02 score=47874
chimera::EdgeEngine::Config s41_ondousdt_tsmom_h8_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_ondousdt_tsmom_h8(s41_ondousdt_tsmom_h8_cfg);
wire_engine(s41_ondousdt_tsmom_h8);

// LINK-ICHI-H12 minPF=2.17 score=43576
chimera::EdgeEngine::Config s41_linkusdt_ichi_h12_cfg{
    .symbol="linkusdt", .tag="LINK-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_linkusdt_ichi_h12(s41_linkusdt_ichi_h12_cfg);
wire_engine(s41_linkusdt_ichi_h12);

// FET-DCH-H4 minPF=3.25 score=19201
chimera::EdgeEngine::Config s41_fetusdt_dch_h4_cfg{
    .symbol="fetusdt", .tag="FET-DCH-H4", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=14400, .lookback=40, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_fetusdt_dch_h4(s41_fetusdt_dch_h4_cfg);
wire_engine(s41_fetusdt_dch_h4);

// PYTH-TSMOM-D1 minPF=2.48 score=83315
chimera::EdgeEngine::Config s41_pythusdt_tsmom_d1_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pythusdt_tsmom_d1(s41_pythusdt_tsmom_d1_cfg);
wire_engine(s41_pythusdt_tsmom_d1);

// SUI-BOPB-H1 minPF=2.35 score=14953
chimera::EdgeEngine::Config s41_suiusdt_bopb_h1_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H1", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_suiusdt_bopb_h1(s41_suiusdt_bopb_h1_cfg);
wire_engine(s41_suiusdt_bopb_h1);

// IMX-TSMOM-H6 minPF=2.10 score=102095
chimera::EdgeEngine::Config s41_imxusdt_tsmom_h6_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_imxusdt_tsmom_h6(s41_imxusdt_tsmom_h6_cfg);
wire_engine(s41_imxusdt_tsmom_h6);

// COMP-SR-H12 minPF=2.61 score=47194
chimera::EdgeEngine::Config s41_compusdt_sr_h12_cfg{
    .symbol="compusdt", .tag="COMP-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=18, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_compusdt_sr_h12(s41_compusdt_sr_h12_cfg);
wire_engine(s41_compusdt_sr_h12);

// LDO-ICHI-H12 minPF=2.18 score=41784
chimera::EdgeEngine::Config s41_ldousdt_ichi_h12_cfg{
    .symbol="ldousdt", .tag="LDO-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_ldousdt_ichi_h12(s41_ldousdt_ichi_h12_cfg);
wire_engine(s41_ldousdt_ichi_h12);

// LDO-TSMOM-D2 minPF=2.32 score=54480
chimera::EdgeEngine::Config s41_ldousdt_tsmom_d2_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_ldousdt_tsmom_d2(s41_ldousdt_tsmom_d2_cfg);
wire_engine(s41_ldousdt_tsmom_d2);

// TON-ICHI-H6 minPF=2.73 score=35321
chimera::EdgeEngine::Config s41_tonusdt_ichi_h6_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_tonusdt_ichi_h6(s41_tonusdt_ichi_h6_cfg);
wire_engine(s41_tonusdt_ichi_h6);

// TON-TSMOM-H12 minPF=2.00 score=31654
chimera::EdgeEngine::Config s41_tonusdt_tsmom_h12_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_tonusdt_tsmom_h12(s41_tonusdt_tsmom_h12_cfg);
wire_engine(s41_tonusdt_tsmom_h12);

// CRV-ICHI-H12 minPF=2.18 score=55876
chimera::EdgeEngine::Config s41_crvusdt_ichi_h12_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_crvusdt_ichi_h12(s41_crvusdt_ichi_h12_cfg);
wire_engine(s41_crvusdt_ichi_h12);

// PEPE-TSMOM-D1 minPF=2.48 score=78480
chimera::EdgeEngine::Config s41_pepeusdt_tsmom_d1_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_tsmom_d1(s41_pepeusdt_tsmom_d1_cfg);
wire_engine(s41_pepeusdt_tsmom_d1);

// APT-TSMOM-D2 minPF=2.57 score=53335
chimera::EdgeEngine::Config s41_aptusdt_tsmom_d2_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_aptusdt_tsmom_d2(s41_aptusdt_tsmom_d2_cfg);
wire_engine(s41_aptusdt_tsmom_d2);

// ARB-ICHI-H4 minPF=2.08 score=64154
chimera::EdgeEngine::Config s41_arbusdt_ichi_h4_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_arbusdt_ichi_h4(s41_arbusdt_ichi_h4_cfg);
wire_engine(s41_arbusdt_ichi_h4);

// ICP-TSMOM-D2 minPF=2.16 score=63505
chimera::EdgeEngine::Config s41_icpusdt_tsmom_d2_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=6, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_icpusdt_tsmom_d2(s41_icpusdt_tsmom_d2_cfg);
wire_engine(s41_icpusdt_tsmom_d2);

// ARB-DCH-H4 minPF=2.26 score=20831
chimera::EdgeEngine::Config s41_arbusdt_dch_h4_cfg{
    .symbol="arbusdt", .tag="ARB-DCH-H4", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_arbusdt_dch_h4(s41_arbusdt_dch_h4_cfg);
wire_engine(s41_arbusdt_dch_h4);

// PEPE-ICHI-H8 minPF=2.31 score=67846
chimera::EdgeEngine::Config s41_pepeusdt_ichi_h8_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_ichi_h8(s41_pepeusdt_ichi_h8_cfg);
wire_engine(s41_pepeusdt_ichi_h8);

// TON-ICHI-H12 minPF=2.24 score=27244
chimera::EdgeEngine::Config s41_tonusdt_ichi_h12_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=18, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_tonusdt_ichi_h12(s41_tonusdt_ichi_h12_cfg);
wire_engine(s41_tonusdt_ichi_h12);

// AVAX-TSMOM-H12 minPF=2.00 score=59627
chimera::EdgeEngine::Config s41_avaxusdt_tsmom_h12_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_avaxusdt_tsmom_h12(s41_avaxusdt_tsmom_h12_cfg);
wire_engine(s41_avaxusdt_tsmom_h12);

// COMP-ICHI-H4 minPF=2.01 score=74311
chimera::EdgeEngine::Config s41_compusdt_ichi_h4_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_compusdt_ichi_h4(s41_compusdt_ichi_h4_cfg);
wire_engine(s41_compusdt_ichi_h4);

// MANA-ICHI-H4 minPF=2.20 score=61459
chimera::EdgeEngine::Config s41_manausdt_ichi_h4_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_manausdt_ichi_h4(s41_manausdt_ichi_h4_cfg);
wire_engine(s41_manausdt_ichi_h4);

// ICP-TSMOM-H3 minPF=2.11 score=143574
chimera::EdgeEngine::Config s41_icpusdt_tsmom_h3_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_icpusdt_tsmom_h3(s41_icpusdt_tsmom_h3_cfg);
wire_engine(s41_icpusdt_tsmom_h3);

// INJ-ICHI-H8 minPF=2.26 score=63965
chimera::EdgeEngine::Config s41_injusdt_ichi_h8_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_injusdt_ichi_h8(s41_injusdt_ichi_h8_cfg);
wire_engine(s41_injusdt_ichi_h8);

// NEAR-ICHI-H3 minPF=2.04 score=73440
chimera::EdgeEngine::Config s41_nearusdt_ichi_h3_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_nearusdt_ichi_h3(s41_nearusdt_ichi_h3_cfg);
wire_engine(s41_nearusdt_ichi_h3);

// INJ-BOPB-H8 minPF=2.00 score=27419
chimera::EdgeEngine::Config s41_injusdt_bopb_h8_cfg{
    .symbol="injusdt", .tag="INJ-BOPB-H8", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=28800, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_injusdt_bopb_h8(s41_injusdt_bopb_h8_cfg);
wire_engine(s41_injusdt_bopb_h8);

// PEPE-TSMOM-H6 minPF=2.10 score=109168
chimera::EdgeEngine::Config s41_pepeusdt_tsmom_h6_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_tsmom_h6(s41_pepeusdt_tsmom_h6_cfg);
wire_engine(s41_pepeusdt_tsmom_h6);

// LDO-TSMOM-D1 minPF=2.15 score=67144
chimera::EdgeEngine::Config s41_ldousdt_tsmom_d1_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_ldousdt_tsmom_d1(s41_ldousdt_tsmom_d1_cfg);
wire_engine(s41_ldousdt_tsmom_d1);

// TON-ICHI-H8 minPF=2.15 score=27700
chimera::EdgeEngine::Config s41_tonusdt_ichi_h8_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=12, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_tonusdt_ichi_h8(s41_tonusdt_ichi_h8_cfg);
wire_engine(s41_tonusdt_ichi_h8);

// PEPE-ICHI-H12 minPF=2.22 score=51905
chimera::EdgeEngine::Config s41_pepeusdt_ichi_h12_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=18, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_ichi_h12(s41_pepeusdt_ichi_h12_cfg);
wire_engine(s41_pepeusdt_ichi_h12);

// NEAR-ICHI-H4 minPF=2.03 score=72380
chimera::EdgeEngine::Config s41_nearusdt_ichi_h4_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=18, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_nearusdt_ichi_h4(s41_nearusdt_ichi_h4_cfg);
wire_engine(s41_nearusdt_ichi_h4);

// COMP-ICHI-H6 minPF=3.31 score=80611
chimera::EdgeEngine::Config s41_compusdt_ichi_h6_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_compusdt_ichi_h6(s41_compusdt_ichi_h6_cfg);
wire_engine(s41_compusdt_ichi_h6);

// ATOM-ICHI-H3 minPF=2.06 score=59528
chimera::EdgeEngine::Config s41_atomusdt_ichi_h3_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_atomusdt_ichi_h3(s41_atomusdt_ichi_h3_cfg);
wire_engine(s41_atomusdt_ichi_h3);

// DOGE-TSMOM-D1 minPF=2.79 score=63684
chimera::EdgeEngine::Config s41_dogeusdt_tsmom_d1_cfg{
    .symbol="dogeusdt", .tag="DOGE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_dogeusdt_tsmom_d1(s41_dogeusdt_tsmom_d1_cfg);
wire_engine(s41_dogeusdt_tsmom_d1);

// MANA-ICHI-H6 minPF=2.40 score=56330
chimera::EdgeEngine::Config s41_manausdt_ichi_h6_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_manausdt_ichi_h6(s41_manausdt_ichi_h6_cfg);
wire_engine(s41_manausdt_ichi_h6);

// TON-TSMOM-D2 minPF=2.00 score=22659
chimera::EdgeEngine::Config s41_tonusdt_tsmom_d2_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_tonusdt_tsmom_d2(s41_tonusdt_tsmom_d2_cfg);
wire_engine(s41_tonusdt_tsmom_d2);

// IMX-ICHI-H4 minPF=2.28 score=76119
chimera::EdgeEngine::Config s41_imxusdt_ichi_h4_cfg{
    .symbol="imxusdt", .tag="IMX-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_imxusdt_ichi_h4(s41_imxusdt_ichi_h4_cfg);
wire_engine(s41_imxusdt_ichi_h4);

// PYTH-TSMOM-D2 minPF=2.12 score=64635
chimera::EdgeEngine::Config s41_pythusdt_tsmom_d2_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pythusdt_tsmom_d2(s41_pythusdt_tsmom_d2_cfg);
wire_engine(s41_pythusdt_tsmom_d2);

// PEPE-ICHI-H3 minPF=2.09 score=90226
chimera::EdgeEngine::Config s41_pepeusdt_ichi_h3_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_ichi_h3(s41_pepeusdt_ichi_h3_cfg);
wire_engine(s41_pepeusdt_ichi_h3);

// TON-ICHI-H2 minPF=2.06 score=40782
chimera::EdgeEngine::Config s41_tonusdt_ichi_h2_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_tonusdt_ichi_h2(s41_tonusdt_ichi_h2_cfg);
wire_engine(s41_tonusdt_ichi_h2);

// PEPE-TSMOM-H8 minPF=2.29 score=112501
chimera::EdgeEngine::Config s41_pepeusdt_tsmom_h8_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_tsmom_h8(s41_pepeusdt_tsmom_h8_cfg);
wire_engine(s41_pepeusdt_tsmom_h8);

// CRV-TSMOM-D2 minPF=2.01 score=54325
chimera::EdgeEngine::Config s41_crvusdt_tsmom_d2_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_crvusdt_tsmom_d2(s41_crvusdt_tsmom_d2_cfg);
wire_engine(s41_crvusdt_tsmom_d2);

// CRV-ICHI-H8 minPF=2.00 score=60083
chimera::EdgeEngine::Config s41_crvusdt_ichi_h8_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_crvusdt_ichi_h8(s41_crvusdt_ichi_h8_cfg);
wire_engine(s41_crvusdt_ichi_h8);

// SUI-DT-H12 minPF=2.05 score=23393
chimera::EdgeEngine::Config s41_suiusdt_dt_h12_cfg{
    .symbol="suiusdt", .tag="SUI-DT-H12", .kind=chimera::StrategyKind::DUAL_THRUST,
    .tf_secs=43200, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_suiusdt_dt_h12(s41_suiusdt_dt_h12_cfg);
wire_engine(s41_suiusdt_dt_h12);

// MANA-TSMOM-H6 minPF=2.06 score=102244
chimera::EdgeEngine::Config s41_manausdt_tsmom_h6_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_manausdt_tsmom_h6(s41_manausdt_tsmom_h6_cfg);
wire_engine(s41_manausdt_tsmom_h6);

// BNB-BOPB-H6 minPF=2.76 score=13709
chimera::EdgeEngine::Config s41_bnbusdt_bopb_h6_cfg{
    .symbol="bnbusdt", .tag="BNB-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_bnbusdt_bopb_h6(s41_bnbusdt_bopb_h6_cfg);
wire_engine(s41_bnbusdt_bopb_h6);

// ICP-TSMOM-D1 minPF=2.19 score=60065
chimera::EdgeEngine::Config s41_icpusdt_tsmom_d1_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_icpusdt_tsmom_d1(s41_icpusdt_tsmom_d1_cfg);
wire_engine(s41_icpusdt_tsmom_d1);

// TRX-ICHI-D1 minPF=3.95 score=41492
chimera::EdgeEngine::Config s41_trxusdt_ichi_d1_cfg{
    .symbol="trxusdt", .tag="TRX-ICHI-D1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=86400, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_trxusdt_ichi_d1(s41_trxusdt_ichi_d1_cfg);
wire_engine(s41_trxusdt_ichi_d1);

// PEPE-TSMOM-D2 minPF=2.57 score=67503
chimera::EdgeEngine::Config s41_pepeusdt_tsmom_d2_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_tsmom_d2(s41_pepeusdt_tsmom_d2_cfg);
wire_engine(s41_pepeusdt_tsmom_d2);

// ONDO-ICHI-H6 minPF=2.27 score=22901
chimera::EdgeEngine::Config s41_ondousdt_ichi_h6_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_ondousdt_ichi_h6(s41_ondousdt_ichi_h6_cfg);
wire_engine(s41_ondousdt_ichi_h6);

// ICP-TSMOM-H2 minPF=2.00 score=148184
chimera::EdgeEngine::Config s41_icpusdt_tsmom_h2_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_icpusdt_tsmom_h2(s41_icpusdt_tsmom_h2_cfg);
wire_engine(s41_icpusdt_tsmom_h2);

// TON-ICHI-H3 minPF=2.25 score=37036
chimera::EdgeEngine::Config s41_tonusdt_ichi_h3_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_tonusdt_ichi_h3(s41_tonusdt_ichi_h3_cfg);
wire_engine(s41_tonusdt_ichi_h3);

// RENDER-TSMOM-D1 minPF=2.50 score=136405
chimera::EdgeEngine::Config s41_renderusdt_tsmom_d1_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_renderusdt_tsmom_d1(s41_renderusdt_tsmom_d1_cfg);
wire_engine(s41_renderusdt_tsmom_d1);

// RENDER-TSMOM-D3 minPF=2.93 score=127029
chimera::EdgeEngine::Config s41_renderusdt_tsmom_d3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-D3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=259200, .lookback=60, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_renderusdt_tsmom_d3(s41_renderusdt_tsmom_d3_cfg);
wire_engine(s41_renderusdt_tsmom_d3);

// RENDER-BOPB-H4 minPF=2.61 score=41783
chimera::EdgeEngine::Config s41_renderusdt_bopb_h4_cfg{
    .symbol="renderusdt", .tag="RENDER-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_renderusdt_bopb_h4(s41_renderusdt_bopb_h4_cfg);
wire_engine(s41_renderusdt_bopb_h4);

// RENDER-TSMOM-H6 minPF=2.38 score=213700
chimera::EdgeEngine::Config s41_renderusdt_tsmom_h6_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=18, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_renderusdt_tsmom_h6(s41_renderusdt_tsmom_h6_cfg);
wire_engine(s41_renderusdt_tsmom_h6);

// RENDER-ICHI-H6 minPF=2.46 score=130620
chimera::EdgeEngine::Config s41_renderusdt_ichi_h6_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_renderusdt_ichi_h6(s41_renderusdt_ichi_h6_cfg);
wire_engine(s41_renderusdt_ichi_h6);

// RENDER-SR-H12 minPF=2.33 score=57309
chimera::EdgeEngine::Config s41_renderusdt_sr_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_renderusdt_sr_h12(s41_renderusdt_sr_h12_cfg);
wire_engine(s41_renderusdt_sr_h12);

// ─── PUSH_BACK BLOCKS ─────
g_slots.push_back({chimera::SYM_FET, &s41_fetusdt_tsmom_h3, "fetusdt", 10800, "FET-TSMOM-H3", 2.35, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ICP, &s41_icpusdt_ichi_h2, "icpusdt", 7200, "ICP-ICHI-H2", 2.75, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_SEI, &s41_seiusdt_boll_h2, "seiusdt", 7200, "SEI-BOLL-H2", 2.23, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_FET, &s41_fetusdt_ichi_h12, "fetusdt", 43200, "FET-ICHI-H12", 2.63, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_MANA, &s41_manausdt_ichi_h12, "manausdt", 43200, "MANA-ICHI-H12", 2.00, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_COMP, &s41_compusdt_tsmom_d1, "compusdt", 86400, "COMP-TSMOM-D1", 2.23, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ICP, &s41_icpusdt_ichi_h3, "icpusdt", 10800, "ICP-ICHI-H3", 2.10, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ONDO, &s41_ondousdt_tsmom_h8, "ondousdt", 28800, "ONDO-TSMOM-H8", 2.02, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_LINK, &s41_linkusdt_ichi_h12, "linkusdt", 43200, "LINK-ICHI-H12", 2.17, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_FET, &s41_fetusdt_dch_h4, "fetusdt", 14400, "FET-DCH-H4", 3.25, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PYTH, &s41_pythusdt_tsmom_d1, "pythusdt", 86400, "PYTH-TSMOM-D1", 2.48, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_SUI, &s41_suiusdt_bopb_h1, "suiusdt", 3600, "SUI-BOPB-H1", 2.35, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_IMX, &s41_imxusdt_tsmom_h6, "imxusdt", 21600, "IMX-TSMOM-H6", 2.10, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_COMP, &s41_compusdt_sr_h12, "compusdt", 43200, "COMP-SR-H12", 2.61, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_LDO, &s41_ldousdt_ichi_h12, "ldousdt", 43200, "LDO-ICHI-H12", 2.18, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_LDO, &s41_ldousdt_tsmom_d2, "ldousdt", 172800, "LDO-TSMOM-D2", 2.32, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TON, &s41_tonusdt_ichi_h6, "tonusdt", 21600, "TON-ICHI-H6", 2.73, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TON, &s41_tonusdt_tsmom_h12, "tonusdt", 43200, "TON-TSMOM-H12", 2.00, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_CRV, &s41_crvusdt_ichi_h12, "crvusdt", 43200, "CRV-ICHI-H12", 2.18, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PEPE, &s41_pepeusdt_tsmom_d1, "pepeusdt", 86400, "PEPE-TSMOM-D1", 2.48, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_APT, &s41_aptusdt_tsmom_d2, "aptusdt", 172800, "APT-TSMOM-D2", 2.57, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ARB, &s41_arbusdt_ichi_h4, "arbusdt", 14400, "ARB-ICHI-H4", 2.08, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ICP, &s41_icpusdt_tsmom_d2, "icpusdt", 172800, "ICP-TSMOM-D2", 2.16, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ARB, &s41_arbusdt_dch_h4, "arbusdt", 14400, "ARB-DCH-H4", 2.26, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PEPE, &s41_pepeusdt_ichi_h8, "pepeusdt", 28800, "PEPE-ICHI-H8", 2.31, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TON, &s41_tonusdt_ichi_h12, "tonusdt", 43200, "TON-ICHI-H12", 2.24, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_AVAX, &s41_avaxusdt_tsmom_h12, "avaxusdt", 43200, "AVAX-TSMOM-H12", 2.00, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_COMP, &s41_compusdt_ichi_h4, "compusdt", 14400, "COMP-ICHI-H4", 2.01, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_MANA, &s41_manausdt_ichi_h4, "manausdt", 14400, "MANA-ICHI-H4", 2.20, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ICP, &s41_icpusdt_tsmom_h3, "icpusdt", 10800, "ICP-TSMOM-H3", 2.11, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_INJ, &s41_injusdt_ichi_h8, "injusdt", 28800, "INJ-ICHI-H8", 2.26, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_NEAR, &s41_nearusdt_ichi_h3, "nearusdt", 10800, "NEAR-ICHI-H3", 2.04, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_INJ, &s41_injusdt_bopb_h8, "injusdt", 28800, "INJ-BOPB-H8", 2.00, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PEPE, &s41_pepeusdt_tsmom_h6, "pepeusdt", 21600, "PEPE-TSMOM-H6", 2.10, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_LDO, &s41_ldousdt_tsmom_d1, "ldousdt", 86400, "LDO-TSMOM-D1", 2.15, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TON, &s41_tonusdt_ichi_h8, "tonusdt", 28800, "TON-ICHI-H8", 2.15, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PEPE, &s41_pepeusdt_ichi_h12, "pepeusdt", 43200, "PEPE-ICHI-H12", 2.22, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_NEAR, &s41_nearusdt_ichi_h4, "nearusdt", 14400, "NEAR-ICHI-H4", 2.03, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_COMP, &s41_compusdt_ichi_h6, "compusdt", 21600, "COMP-ICHI-H6", 3.31, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ATOM, &s41_atomusdt_ichi_h3, "atomusdt", 10800, "ATOM-ICHI-H3", 2.06, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_DOGE, &s41_dogeusdt_tsmom_d1, "dogeusdt", 86400, "DOGE-TSMOM-D1", 2.79, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_MANA, &s41_manausdt_ichi_h6, "manausdt", 21600, "MANA-ICHI-H6", 2.40, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TON, &s41_tonusdt_tsmom_d2, "tonusdt", 172800, "TON-TSMOM-D2", 2.00, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_IMX, &s41_imxusdt_ichi_h4, "imxusdt", 14400, "IMX-ICHI-H4", 2.28, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PYTH, &s41_pythusdt_tsmom_d2, "pythusdt", 172800, "PYTH-TSMOM-D2", 2.12, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PEPE, &s41_pepeusdt_ichi_h3, "pepeusdt", 10800, "PEPE-ICHI-H3", 2.09, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TON, &s41_tonusdt_ichi_h2, "tonusdt", 7200, "TON-ICHI-H2", 2.06, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PEPE, &s41_pepeusdt_tsmom_h8, "pepeusdt", 28800, "PEPE-TSMOM-H8", 2.29, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_CRV, &s41_crvusdt_tsmom_d2, "crvusdt", 172800, "CRV-TSMOM-D2", 2.01, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_CRV, &s41_crvusdt_ichi_h8, "crvusdt", 28800, "CRV-ICHI-H8", 2.00, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_SUI, &s41_suiusdt_dt_h12, "suiusdt", 43200, "SUI-DT-H12", 2.05, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_MANA, &s41_manausdt_tsmom_h6, "manausdt", 21600, "MANA-TSMOM-H6", 2.06, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_BNB, &s41_bnbusdt_bopb_h6, "bnbusdt", 21600, "BNB-BOPB-H6", 2.76, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ICP, &s41_icpusdt_tsmom_d1, "icpusdt", 86400, "ICP-TSMOM-D1", 2.19, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TRX, &s41_trxusdt_ichi_d1, "trxusdt", 86400, "TRX-ICHI-D1", 3.95, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_PEPE, &s41_pepeusdt_tsmom_d2, "pepeusdt", 172800, "PEPE-TSMOM-D2", 2.57, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ONDO, &s41_ondousdt_ichi_h6, "ondousdt", 21600, "ONDO-ICHI-H6", 2.27, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_ICP, &s41_icpusdt_tsmom_h2, "icpusdt", 7200, "ICP-TSMOM-H2", 2.00, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_TON, &s41_tonusdt_ichi_h3, "tonusdt", 10800, "TON-ICHI-H3", 2.25, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_RENDER, &s41_renderusdt_tsmom_d1, "renderusdt", 86400, "RENDER-TSMOM-D1", 2.50, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_RENDER, &s41_renderusdt_tsmom_d3, "renderusdt", 259200, "RENDER-TSMOM-D3", 2.93, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_RENDER, &s41_renderusdt_bopb_h4, "renderusdt", 14400, "RENDER-BOPB-H4", 2.61, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_RENDER, &s41_renderusdt_tsmom_h6, "renderusdt", 21600, "RENDER-TSMOM-H6", 2.38, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_RENDER, &s41_renderusdt_ichi_h6, "renderusdt", 21600, "RENDER-ICHI-H6", 2.46, 0.0, 100, 730, 41});
g_slots.push_back({chimera::SYM_RENDER, &s41_renderusdt_sr_h12, "renderusdt", 43200, "RENDER-SR-H12", 2.33, 0.0, 100, 730, 41});
