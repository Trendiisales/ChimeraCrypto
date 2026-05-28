// S38b ADDS — 91 new engines + wire_engine + pyramid_xlow per slot.
// All survive PF>=1.3 + net_bp>0 in 4-window WF; 99.2% gain from pyramid_xlow.
// ─── CONFIG + ENGINE + WIRE + PYRAMID ────────────────────────────────
// XRP-TSMOM-D1: 134d PF=2.29 180d PF=2.01 365d PF=2.31 730d PF=1.95
chimera::EdgeEngine::Config s38_xrpusdt_tsmom_d1_cfg{
    .symbol="xrpusdt", .tag="XRP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=10, .hold_bars=6, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_xrpusdt_tsmom_d1(s38_xrpusdt_tsmom_d1_cfg);
wire_engine(s38_xrpusdt_tsmom_d1);

// XRP-ICHI-H8: 134d PF=1.46 180d PF=1.45 365d PF=2.35 730d PF=2.75
chimera::EdgeEngine::Config s38_xrpusdt_ichi_h8_cfg{
    .symbol="xrpusdt", .tag="XRP-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_xrpusdt_ichi_h8(s38_xrpusdt_ichi_h8_cfg);
wire_engine(s38_xrpusdt_ichi_h8);

// XRP-ICHI-H4: 134d PF=2.42 180d PF=1.95 365d PF=1.80 730d PF=1.88
chimera::EdgeEngine::Config s38_xrpusdt_ichi_h4_cfg{
    .symbol="xrpusdt", .tag="XRP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_xrpusdt_ichi_h4(s38_xrpusdt_ichi_h4_cfg);
wire_engine(s38_xrpusdt_ichi_h4);

// WIF-TSMOM-D1: 134d PF=2.96 180d PF=2.92 365d PF=4.42 730d PF=2.93
chimera::EdgeEngine::Config s38_wifusdt_tsmom_d1_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=40, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_tsmom_d1(s38_wifusdt_tsmom_d1_cfg);
wire_engine(s38_wifusdt_tsmom_d1);

// WIF-TSMOM-H12: 134d PF=2.64 180d PF=2.26 365d PF=2.83 730d PF=2.01
chimera::EdgeEngine::Config s38_wifusdt_tsmom_h12_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=10, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_tsmom_h12(s38_wifusdt_tsmom_h12_cfg);
wire_engine(s38_wifusdt_tsmom_h12);

// WIF-TSMOM-H1: 134d PF=1.52 180d PF=1.44 365d PF=1.51 730d PF=1.67
chimera::EdgeEngine::Config s38_wifusdt_tsmom_h1_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_tsmom_h1(s38_wifusdt_tsmom_h1_cfg);
wire_engine(s38_wifusdt_tsmom_h1);

// WIF-TSMOM-H8: 134d PF=2.87 180d PF=3.89 365d PF=3.42 730d PF=2.76
chimera::EdgeEngine::Config s38_wifusdt_tsmom_h8_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=40, .hold_bars=6, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_tsmom_h8(s38_wifusdt_tsmom_h8_cfg);
wire_engine(s38_wifusdt_tsmom_h8);

// WIF-TSMOM-H4: 134d PF=2.21 180d PF=1.87 365d PF=1.61 730d PF=1.69
chimera::EdgeEngine::Config s38_wifusdt_tsmom_h4_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_tsmom_h4(s38_wifusdt_tsmom_h4_cfg);
wire_engine(s38_wifusdt_tsmom_h4);

// WIF-ICHI-H1: 134d PF=1.51 180d PF=1.36 365d PF=1.32 730d PF=1.72
chimera::EdgeEngine::Config s38_wifusdt_ichi_h1_cfg{
    .symbol="wifusdt", .tag="WIF-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_ichi_h1(s38_wifusdt_ichi_h1_cfg);
wire_engine(s38_wifusdt_ichi_h1);

// WIF-ICHI-H8: 134d PF=2.30 180d PF=1.36 365d PF=2.84 730d PF=2.38
chimera::EdgeEngine::Config s38_wifusdt_ichi_h8_cfg{
    .symbol="wifusdt", .tag="WIF-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=40, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_ichi_h8(s38_wifusdt_ichi_h8_cfg);
wire_engine(s38_wifusdt_ichi_h8);

// WIF-DT-H8: 134d PF=2.24 180d PF=1.52 365d PF=1.94 730d PF=1.60
chimera::EdgeEngine::Config s38_wifusdt_dt_h8_cfg{
    .symbol="wifusdt", .tag="WIF-DT-H8", .kind=chimera::StrategyKind::DUAL_THRUST,
    .tf_secs=28800, .lookback=40, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_wifusdt_dt_h8(s38_wifusdt_dt_h8_cfg);
wire_engine(s38_wifusdt_dt_h8);

// TIA-TSMOM-D1: 134d PF=166.51 180d PF=157.85 365d PF=2.28 730d PF=1.98
chimera::EdgeEngine::Config s38_tiausdt_tsmom_d1_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=20, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_tiausdt_tsmom_d1(s38_tiausdt_tsmom_d1_cfg);
wire_engine(s38_tiausdt_tsmom_d1);

// TIA-TSMOM-H1: 134d PF=1.88 180d PF=1.73 365d PF=1.61 730d PF=1.61
chimera::EdgeEngine::Config s38_tiausdt_tsmom_h1_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_tiausdt_tsmom_h1(s38_tiausdt_tsmom_h1_cfg);
wire_engine(s38_tiausdt_tsmom_h1);

// TIA-TSMOM-H4: 134d PF=2.68 180d PF=2.66 365d PF=2.47 730d PF=2.19
chimera::EdgeEngine::Config s38_tiausdt_tsmom_h4_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=40, .hold_bars=6, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_tiausdt_tsmom_h4(s38_tiausdt_tsmom_h4_cfg);
wire_engine(s38_tiausdt_tsmom_h4);

// TIA-ICHI-H12: 134d PF=5.83 180d PF=5.83 365d PF=1.30 730d PF=1.35
chimera::EdgeEngine::Config s38_tiausdt_ichi_h12_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_tiausdt_ichi_h12(s38_tiausdt_ichi_h12_cfg);
wire_engine(s38_tiausdt_ichi_h12);

// TIA-ICHI-H1: 134d PF=1.52 180d PF=1.55 365d PF=1.43 730d PF=1.70
chimera::EdgeEngine::Config s38_tiausdt_ichi_h1_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=10, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_tiausdt_ichi_h1(s38_tiausdt_ichi_h1_cfg);
wire_engine(s38_tiausdt_ichi_h1);

// TIA-ICHI-H8: 134d PF=3.36 180d PF=2.82 365d PF=2.05 730d PF=1.37
chimera::EdgeEngine::Config s38_tiausdt_ichi_h8_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=40, .hold_bars=6, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_tiausdt_ichi_h8(s38_tiausdt_ichi_h8_cfg);
wire_engine(s38_tiausdt_ichi_h8);

// TIA-ICHI-H4: 134d PF=2.51 180d PF=2.55 365d PF=2.16 730d PF=2.04
chimera::EdgeEngine::Config s38_tiausdt_ichi_h4_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_tiausdt_ichi_h4(s38_tiausdt_ichi_h4_cfg);
wire_engine(s38_tiausdt_ichi_h4);

// SUI-TSMOM-D1: 134d PF=2.36 180d PF=3.29 365d PF=2.42 730d PF=2.72
chimera::EdgeEngine::Config s38_suiusdt_tsmom_d1_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=10, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_tsmom_d1(s38_suiusdt_tsmom_d1_cfg);
wire_engine(s38_suiusdt_tsmom_d1);

// SUI-TSMOM-H12: 134d PF=1.59 180d PF=1.54 365d PF=1.72 730d PF=1.80
chimera::EdgeEngine::Config s38_suiusdt_tsmom_h12_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=20, .hold_bars=6, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_tsmom_h12(s38_suiusdt_tsmom_h12_cfg);
wire_engine(s38_suiusdt_tsmom_h12);

// SUI-TSMOM-H1: 134d PF=1.34 180d PF=1.48 365d PF=1.45 730d PF=1.59
chimera::EdgeEngine::Config s38_suiusdt_tsmom_h1_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_tsmom_h1(s38_suiusdt_tsmom_h1_cfg);
wire_engine(s38_suiusdt_tsmom_h1);

// SUI-ICHI-H8: 134d PF=5.57 180d PF=5.57 365d PF=2.54 730d PF=2.59
chimera::EdgeEngine::Config s38_suiusdt_ichi_h8_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_ichi_h8(s38_suiusdt_ichi_h8_cfg);
wire_engine(s38_suiusdt_ichi_h8);

// SUI-ICHI-H4: 134d PF=2.68 180d PF=2.49 365d PF=2.36 730d PF=1.73
chimera::EdgeEngine::Config s38_suiusdt_ichi_h4_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=40, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_ichi_h4(s38_suiusdt_ichi_h4_cfg);
wire_engine(s38_suiusdt_ichi_h4);

// SUI-DT-H12: 134d PF=2.05 180d PF=1.90 365d PF=1.58 730d PF=1.30
chimera::EdgeEngine::Config s38_suiusdt_dt_h12_cfg{
    .symbol="suiusdt", .tag="SUI-DT-H12", .kind=chimera::StrategyKind::DUAL_THRUST,
    .tf_secs=43200, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_dt_h12(s38_suiusdt_dt_h12_cfg);
wire_engine(s38_suiusdt_dt_h12);

// SUI-BOPB-H1: 134d PF=2.35 180d PF=2.90 365d PF=2.01 730d PF=1.61
chimera::EdgeEngine::Config s38_suiusdt_bopb_h1_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H1", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_bopb_h1(s38_suiusdt_bopb_h1_cfg);
wire_engine(s38_suiusdt_bopb_h1);

// SUI-BOPB-H4: 134d PF=3.30 180d PF=2.23 365d PF=2.13 730d PF=1.74
chimera::EdgeEngine::Config s38_suiusdt_bopb_h4_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=10, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_suiusdt_bopb_h4(s38_suiusdt_bopb_h4_cfg);
wire_engine(s38_suiusdt_bopb_h4);

// SOL-TSMOM-H8: 134d PF=4.67 180d PF=2.59 365d PF=2.74 730d PF=2.28
chimera::EdgeEngine::Config s38_solusdt_tsmom_h8_cfg{
    .symbol="solusdt", .tag="SOL-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=20, .hold_bars=6, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_solusdt_tsmom_h8(s38_solusdt_tsmom_h8_cfg);
wire_engine(s38_solusdt_tsmom_h8);

// SOL-ICHI-H12: 134d PF=1.38 180d PF=1.38 365d PF=1.79 730d PF=1.96
chimera::EdgeEngine::Config s38_solusdt_ichi_h12_cfg{
    .symbol="solusdt", .tag="SOL-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=20, .hold_bars=6, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_solusdt_ichi_h12(s38_solusdt_ichi_h12_cfg);
wire_engine(s38_solusdt_ichi_h12);

// SOL-ICHI-H4: 134d PF=1.64 180d PF=1.39 365d PF=2.03 730d PF=2.23
chimera::EdgeEngine::Config s38_solusdt_ichi_h4_cfg{
    .symbol="solusdt", .tag="SOL-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_solusdt_ichi_h4(s38_solusdt_ichi_h4_cfg);
wire_engine(s38_solusdt_ichi_h4);

// PEPE-TSMOM-D1: 134d PF=3.95 180d PF=3.26 365d PF=3.30 730d PF=2.32
chimera::EdgeEngine::Config s38_pepeusdt_tsmom_d1_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=10, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_tsmom_d1(s38_pepeusdt_tsmom_d1_cfg);
wire_engine(s38_pepeusdt_tsmom_d1);

// PEPE-TSMOM-H12: 134d PF=3.24 180d PF=2.73 365d PF=1.81 730d PF=1.98
chimera::EdgeEngine::Config s38_pepeusdt_tsmom_h12_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_tsmom_h12(s38_pepeusdt_tsmom_h12_cfg);
wire_engine(s38_pepeusdt_tsmom_h12);

// PEPE-TSMOM-H8: 134d PF=2.52 180d PF=2.43 365d PF=2.70 730d PF=2.67
chimera::EdgeEngine::Config s38_pepeusdt_tsmom_h8_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_tsmom_h8(s38_pepeusdt_tsmom_h8_cfg);
wire_engine(s38_pepeusdt_tsmom_h8);

// PEPE-TSMOM-H4: 134d PF=2.19 180d PF=2.29 365d PF=1.65 730d PF=1.93
chimera::EdgeEngine::Config s38_pepeusdt_tsmom_h4_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_tsmom_h4(s38_pepeusdt_tsmom_h4_cfg);
wire_engine(s38_pepeusdt_tsmom_h4);

// PEPE-SR-H12: 134d PF=3.28 180d PF=2.65 365d PF=1.42 730d PF=1.34
chimera::EdgeEngine::Config s38_pepeusdt_sr_h12_cfg{
    .symbol="pepeusdt", .tag="PEPE-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=40, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_sr_h12(s38_pepeusdt_sr_h12_cfg);
wire_engine(s38_pepeusdt_sr_h12);

// PEPE-ICHI-H12: 134d PF=3.43 180d PF=4.21 365d PF=4.16 730d PF=3.01
chimera::EdgeEngine::Config s38_pepeusdt_ichi_h12_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_ichi_h12(s38_pepeusdt_ichi_h12_cfg);
wire_engine(s38_pepeusdt_ichi_h12);

// PEPE-ICHI-H8: 134d PF=2.25 180d PF=2.34 365d PF=3.31 730d PF=3.16
chimera::EdgeEngine::Config s38_pepeusdt_ichi_h8_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_ichi_h8(s38_pepeusdt_ichi_h8_cfg);
wire_engine(s38_pepeusdt_ichi_h8);

// PEPE-ICHI-H4: 134d PF=1.93 180d PF=1.87 365d PF=1.69 730d PF=2.15
chimera::EdgeEngine::Config s38_pepeusdt_ichi_h4_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_ichi_h4(s38_pepeusdt_ichi_h4_cfg);
wire_engine(s38_pepeusdt_ichi_h4);

// PEPE-DT-H8: 134d PF=1.40 180d PF=1.71 365d PF=1.63 730d PF=1.38
chimera::EdgeEngine::Config s38_pepeusdt_dt_h8_cfg{
    .symbol="pepeusdt", .tag="PEPE-DT-H8", .kind=chimera::StrategyKind::DUAL_THRUST,
    .tf_secs=28800, .lookback=20, .hold_bars=6, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_pepeusdt_dt_h8(s38_pepeusdt_dt_h8_cfg);
wire_engine(s38_pepeusdt_dt_h8);

// ONDO-TSMOM-D1: 134d PF=3.09 180d PF=2.24 365d PF=2.45 730d PF=99.90
chimera::EdgeEngine::Config s38_ondousdt_tsmom_d1_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_tsmom_d1(s38_ondousdt_tsmom_d1_cfg);
wire_engine(s38_ondousdt_tsmom_d1);

// ONDO-TSMOM-H1: 134d PF=1.75 180d PF=1.61 365d PF=1.43 730d PF=1.85
chimera::EdgeEngine::Config s38_ondousdt_tsmom_h1_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_tsmom_h1(s38_ondousdt_tsmom_h1_cfg);
wire_engine(s38_ondousdt_tsmom_h1);

// ONDO-TSMOM-H8: 134d PF=2.24 180d PF=1.94 365d PF=2.28 730d PF=2.87
chimera::EdgeEngine::Config s38_ondousdt_tsmom_h8_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=20, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_tsmom_h8(s38_ondousdt_tsmom_h8_cfg);
wire_engine(s38_ondousdt_tsmom_h8);

// ONDO-TSMOM-H4: 134d PF=2.43 180d PF=2.41 365d PF=2.03 730d PF=2.08
chimera::EdgeEngine::Config s38_ondousdt_tsmom_h4_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=40, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_tsmom_h4(s38_ondousdt_tsmom_h4_cfg);
wire_engine(s38_ondousdt_tsmom_h4);

// ONDO-ICHI-H12: 134d PF=4.36 180d PF=4.36 365d PF=2.27 730d PF=4.36
chimera::EdgeEngine::Config s38_ondousdt_ichi_h12_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_ichi_h12(s38_ondousdt_ichi_h12_cfg);
wire_engine(s38_ondousdt_ichi_h12);

// ONDO-ICHI-H8: 134d PF=1.52 180d PF=1.52 365d PF=1.75 730d PF=1.52
chimera::EdgeEngine::Config s38_ondousdt_ichi_h8_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=10, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_ichi_h8(s38_ondousdt_ichi_h8_cfg);
wire_engine(s38_ondousdt_ichi_h8);

// ONDO-ICHI-H4: 134d PF=3.02 180d PF=3.03 365d PF=2.10 730d PF=2.68
chimera::EdgeEngine::Config s38_ondousdt_ichi_h4_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=40, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_ichi_h4(s38_ondousdt_ichi_h4_cfg);
wire_engine(s38_ondousdt_ichi_h4);

// ONDO-DT-H12: 134d PF=4.24 180d PF=2.69 365d PF=1.64 730d PF=8.39
chimera::EdgeEngine::Config s38_ondousdt_dt_h12_cfg{
    .symbol="ondousdt", .tag="ONDO-DT-H12", .kind=chimera::StrategyKind::DUAL_THRUST,
    .tf_secs=43200, .lookback=10, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ondousdt_dt_h12(s38_ondousdt_dt_h12_cfg);
wire_engine(s38_ondousdt_dt_h12);

// NEAR-TSMOM-H1: 134d PF=1.36 180d PF=1.36 365d PF=1.36 730d PF=1.51
chimera::EdgeEngine::Config s38_nearusdt_tsmom_h1_cfg{
    .symbol="nearusdt", .tag="NEAR-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_nearusdt_tsmom_h1(s38_nearusdt_tsmom_h1_cfg);
wire_engine(s38_nearusdt_tsmom_h1);

// NEAR-ICHI-H1: 134d PF=1.47 180d PF=1.39 365d PF=1.50 730d PF=1.70
chimera::EdgeEngine::Config s38_nearusdt_ichi_h1_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=20, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_nearusdt_ichi_h1(s38_nearusdt_ichi_h1_cfg);
wire_engine(s38_nearusdt_ichi_h1);

// NEAR-ICHI-H4: 134d PF=2.90 180d PF=3.05 365d PF=2.26 730d PF=2.28
chimera::EdgeEngine::Config s38_nearusdt_ichi_h4_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=10, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_nearusdt_ichi_h4(s38_nearusdt_ichi_h4_cfg);
wire_engine(s38_nearusdt_ichi_h4);

// LINK-ICHI-H12: 134d PF=99.90 180d PF=99.90 365d PF=2.47 730d PF=2.15
chimera::EdgeEngine::Config s38_linkusdt_ichi_h12_cfg{
    .symbol="linkusdt", .tag="LINK-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_linkusdt_ichi_h12(s38_linkusdt_ichi_h12_cfg);
wire_engine(s38_linkusdt_ichi_h12);

// LINK-ICHI-H4: 134d PF=1.51 180d PF=1.36 365d PF=2.59 730d PF=1.97
chimera::EdgeEngine::Config s38_linkusdt_ichi_h4_cfg{
    .symbol="linkusdt", .tag="LINK-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=10, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_linkusdt_ichi_h4(s38_linkusdt_ichi_h4_cfg);
wire_engine(s38_linkusdt_ichi_h4);

// FET-TSMOM-H1: 134d PF=1.40 180d PF=1.43 365d PF=1.47 730d PF=1.41
chimera::EdgeEngine::Config s38_fetusdt_tsmom_h1_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_tsmom_h1(s38_fetusdt_tsmom_h1_cfg);
wire_engine(s38_fetusdt_tsmom_h1);

// FET-TSMOM-H4: 134d PF=2.85 180d PF=2.54 365d PF=2.03 730d PF=1.87
chimera::EdgeEngine::Config s38_fetusdt_tsmom_h4_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_tsmom_h4(s38_fetusdt_tsmom_h4_cfg);
wire_engine(s38_fetusdt_tsmom_h4);

// FET-ICHI-H12: 134d PF=2.63 180d PF=2.57 365d PF=1.39 730d PF=1.64
chimera::EdgeEngine::Config s38_fetusdt_ichi_h12_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_ichi_h12(s38_fetusdt_ichi_h12_cfg);
wire_engine(s38_fetusdt_ichi_h12);

// FET-ICHI-H1: 134d PF=1.36 180d PF=1.37 365d PF=1.37 730d PF=1.40
chimera::EdgeEngine::Config s38_fetusdt_ichi_h1_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_ichi_h1(s38_fetusdt_ichi_h1_cfg);
wire_engine(s38_fetusdt_ichi_h1);

// FET-ICHI-H8: 134d PF=3.20 180d PF=3.24 365d PF=2.05 730d PF=1.71
chimera::EdgeEngine::Config s38_fetusdt_ichi_h8_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_ichi_h8(s38_fetusdt_ichi_h8_cfg);
wire_engine(s38_fetusdt_ichi_h8);

// FET-ICHI-H4: 134d PF=7.71 180d PF=7.46 365d PF=2.75 730d PF=2.45
chimera::EdgeEngine::Config s38_fetusdt_ichi_h4_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=10, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_ichi_h4(s38_fetusdt_ichi_h4_cfg);
wire_engine(s38_fetusdt_ichi_h4);

// FET-DCH-H4: 134d PF=3.25 180d PF=2.35 365d PF=2.41 730d PF=1.57
chimera::EdgeEngine::Config s38_fetusdt_dch_h4_cfg{
    .symbol="fetusdt", .tag="FET-DCH-H4", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=14400, .lookback=40, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_dch_h4(s38_fetusdt_dch_h4_cfg);
wire_engine(s38_fetusdt_dch_h4);

// FET-BOPB-H4: 134d PF=2.04 180d PF=1.44 365d PF=1.56 730d PF=1.74
chimera::EdgeEngine::Config s38_fetusdt_bopb_h4_cfg{
    .symbol="fetusdt", .tag="FET-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_fetusdt_bopb_h4(s38_fetusdt_bopb_h4_cfg);
wire_engine(s38_fetusdt_bopb_h4);

// ETH-TSMOM-H12: 134d PF=1.87 180d PF=1.96 365d PF=2.28 730d PF=2.38
chimera::EdgeEngine::Config s38_ethusdt_tsmom_h12_cfg{
    .symbol="ethusdt", .tag="ETH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=17, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ethusdt_tsmom_h12(s38_ethusdt_tsmom_h12_cfg);
wire_engine(s38_ethusdt_tsmom_h12);

// ETH-ICHI-H4: 134d PF=3.33 180d PF=2.37 365d PF=1.73 730d PF=2.41
chimera::EdgeEngine::Config s38_ethusdt_ichi_h4_cfg{
    .symbol="ethusdt", .tag="ETH-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=17, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_ethusdt_ichi_h4(s38_ethusdt_ichi_h4_cfg);
wire_engine(s38_ethusdt_ichi_h4);

// DOGE-TSMOM-D1: 134d PF=24.00 180d PF=2.90 365d PF=5.22 730d PF=2.90
chimera::EdgeEngine::Config s38_dogeusdt_tsmom_d1_cfg{
    .symbol="dogeusdt", .tag="DOGE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=10, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_dogeusdt_tsmom_d1(s38_dogeusdt_tsmom_d1_cfg);
wire_engine(s38_dogeusdt_tsmom_d1);

// DOGE-TSMOM-H4: 134d PF=1.81 180d PF=1.60 365d PF=1.69 730d PF=1.99
chimera::EdgeEngine::Config s38_dogeusdt_tsmom_h4_cfg{
    .symbol="dogeusdt", .tag="DOGE-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_dogeusdt_tsmom_h4(s38_dogeusdt_tsmom_h4_cfg);
wire_engine(s38_dogeusdt_tsmom_h4);

// DOGE-ICHI-H1: 134d PF=1.38 180d PF=1.32 365d PF=1.34 730d PF=1.51
chimera::EdgeEngine::Config s38_dogeusdt_ichi_h1_cfg{
    .symbol="dogeusdt", .tag="DOGE-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_dogeusdt_ichi_h1(s38_dogeusdt_ichi_h1_cfg);
wire_engine(s38_dogeusdt_ichi_h1);

// DOGE-ICHI-H8: 134d PF=1.81 180d PF=1.67 365d PF=2.38 730d PF=2.19
chimera::EdgeEngine::Config s38_dogeusdt_ichi_h8_cfg{
    .symbol="dogeusdt", .tag="DOGE-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_dogeusdt_ichi_h8(s38_dogeusdt_ichi_h8_cfg);
wire_engine(s38_dogeusdt_ichi_h8);

// DOGE-ICHI-H4: 134d PF=1.73 180d PF=1.43 365d PF=1.72 730d PF=2.01
chimera::EdgeEngine::Config s38_dogeusdt_ichi_h4_cfg{
    .symbol="dogeusdt", .tag="DOGE-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=10, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_dogeusdt_ichi_h4(s38_dogeusdt_ichi_h4_cfg);
wire_engine(s38_dogeusdt_ichi_h4);

// DOGE-BOPB-H4: 134d PF=25.84 180d PF=26.38 365d PF=1.64 730d PF=1.85
chimera::EdgeEngine::Config s38_dogeusdt_bopb_h4_cfg{
    .symbol="dogeusdt", .tag="DOGE-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=10, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_dogeusdt_bopb_h4(s38_dogeusdt_bopb_h4_cfg);
wire_engine(s38_dogeusdt_bopb_h4);

// BTC-ICHI-H8: 134d PF=3.77 180d PF=3.59 365d PF=2.80 730d PF=1.67
chimera::EdgeEngine::Config s38_btcusdt_ichi_h8_cfg{
    .symbol="btcusdt", .tag="BTC-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=10, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=17, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_btcusdt_ichi_h8(s38_btcusdt_ichi_h8_cfg);
wire_engine(s38_btcusdt_ichi_h8);

// BTC-ICHI-H4: 134d PF=2.19 180d PF=2.14 365d PF=1.68 730d PF=2.18
chimera::EdgeEngine::Config s38_btcusdt_ichi_h4_cfg{
    .symbol="btcusdt", .tag="BTC-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=17, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_btcusdt_ichi_h4(s38_btcusdt_ichi_h4_cfg);
wire_engine(s38_btcusdt_ichi_h4);

// BNB-ICHI-H12: 134d PF=1.40 180d PF=1.40 365d PF=1.82 730d PF=1.62
chimera::EdgeEngine::Config s38_bnbusdt_ichi_h12_cfg{
    .symbol="bnbusdt", .tag="BNB-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=10, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_bnbusdt_ichi_h12(s38_bnbusdt_ichi_h12_cfg);
wire_engine(s38_bnbusdt_ichi_h12);

// BNB-ICHI-H8: 134d PF=1.69 180d PF=1.60 365d PF=2.05 730d PF=1.88
chimera::EdgeEngine::Config s38_bnbusdt_ichi_h8_cfg{
    .symbol="bnbusdt", .tag="BNB-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=10, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_bnbusdt_ichi_h8(s38_bnbusdt_ichi_h8_cfg);
wire_engine(s38_bnbusdt_ichi_h8);

// BNB-ICHI-H4: 134d PF=1.67 180d PF=1.73 365d PF=1.41 730d PF=1.58
chimera::EdgeEngine::Config s38_bnbusdt_ichi_h4_cfg{
    .symbol="bnbusdt", .tag="BNB-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_bnbusdt_ichi_h4(s38_bnbusdt_ichi_h4_cfg);
wire_engine(s38_bnbusdt_ichi_h4);

// AVAX-TSMOM-D1: 134d PF=3.11 180d PF=1.74 365d PF=3.20 730d PF=3.10
chimera::EdgeEngine::Config s38_avaxusdt_tsmom_d1_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_avaxusdt_tsmom_d1(s38_avaxusdt_tsmom_d1_cfg);
wire_engine(s38_avaxusdt_tsmom_d1);

// AVAX-TSMOM-H12: 134d PF=15.86 180d PF=3.43 365d PF=3.00 730d PF=1.79
chimera::EdgeEngine::Config s38_avaxusdt_tsmom_h12_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=10, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_avaxusdt_tsmom_h12(s38_avaxusdt_tsmom_h12_cfg);
wire_engine(s38_avaxusdt_tsmom_h12);

// AVAX-ICHI-H8: 134d PF=1.41 180d PF=1.43 365d PF=1.53 730d PF=2.12
chimera::EdgeEngine::Config s38_avaxusdt_ichi_h8_cfg{
    .symbol="avaxusdt", .tag="AVAX-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_avaxusdt_ichi_h8(s38_avaxusdt_ichi_h8_cfg);
wire_engine(s38_avaxusdt_ichi_h8);

// AVAX-ICHI-H4: 134d PF=1.65 180d PF=1.33 365d PF=1.69 730d PF=1.93
chimera::EdgeEngine::Config s38_avaxusdt_ichi_h4_cfg{
    .symbol="avaxusdt", .tag="AVAX-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_avaxusdt_ichi_h4(s38_avaxusdt_ichi_h4_cfg);
wire_engine(s38_avaxusdt_ichi_h4);

// ARB-TSMOM-D1: 134d PF=2.31 180d PF=1.30 365d PF=2.97 730d PF=3.03
chimera::EdgeEngine::Config s38_arbusdt_tsmom_d1_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_tsmom_d1(s38_arbusdt_tsmom_d1_cfg);
wire_engine(s38_arbusdt_tsmom_d1);

// ARB-TSMOM-H12: 134d PF=2.26 180d PF=2.28 365d PF=2.86 730d PF=2.15
chimera::EdgeEngine::Config s38_arbusdt_tsmom_h12_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=20, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_tsmom_h12(s38_arbusdt_tsmom_h12_cfg);
wire_engine(s38_arbusdt_tsmom_h12);

// ARB-TSMOM-H1: 134d PF=1.67 180d PF=1.45 365d PF=1.46 730d PF=1.45
chimera::EdgeEngine::Config s38_arbusdt_tsmom_h1_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_tsmom_h1(s38_arbusdt_tsmom_h1_cfg);
wire_engine(s38_arbusdt_tsmom_h1);

// ARB-TSMOM-H4: 134d PF=2.75 180d PF=2.16 365d PF=2.16 730d PF=2.16
chimera::EdgeEngine::Config s38_arbusdt_tsmom_h4_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=40, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_tsmom_h4(s38_arbusdt_tsmom_h4_cfg);
wire_engine(s38_arbusdt_tsmom_h4);

// ARB-ICHI-H12: 134d PF=3.11 180d PF=3.11 365d PF=1.69 730d PF=1.38
chimera::EdgeEngine::Config s38_arbusdt_ichi_h12_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=40, .hold_bars=6, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_ichi_h12(s38_arbusdt_ichi_h12_cfg);
wire_engine(s38_arbusdt_ichi_h12);

// ARB-ICHI-H1: 134d PF=1.68 180d PF=1.52 365d PF=1.34 730d PF=1.40
chimera::EdgeEngine::Config s38_arbusdt_ichi_h1_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=20, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_ichi_h1(s38_arbusdt_ichi_h1_cfg);
wire_engine(s38_arbusdt_ichi_h1);

// ARB-ICHI-H8: 134d PF=1.59 180d PF=1.57 365d PF=1.91 730d PF=2.11
chimera::EdgeEngine::Config s38_arbusdt_ichi_h8_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_ichi_h8(s38_arbusdt_ichi_h8_cfg);
wire_engine(s38_arbusdt_ichi_h8);

// ARB-ICHI-H4: 134d PF=2.65 180d PF=2.37 365d PF=1.93 730d PF=2.12
chimera::EdgeEngine::Config s38_arbusdt_ichi_h4_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=20, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_ichi_h4(s38_arbusdt_ichi_h4_cfg);
wire_engine(s38_arbusdt_ichi_h4);

// ARB-DCH-H4: 134d PF=2.26 180d PF=2.61 365d PF=1.52 730d PF=1.79
chimera::EdgeEngine::Config s38_arbusdt_dch_h4_cfg{
    .symbol="arbusdt", .tag="ARB-DCH-H4", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=14400, .lookback=20, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_arbusdt_dch_h4(s38_arbusdt_dch_h4_cfg);
wire_engine(s38_arbusdt_dch_h4);

// APT-TSMOM-D1: 134d PF=2.34 180d PF=2.66 365d PF=3.36 730d PF=2.33
chimera::EdgeEngine::Config s38_aptusdt_tsmom_d1_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=10, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_aptusdt_tsmom_d1(s38_aptusdt_tsmom_d1_cfg);
wire_engine(s38_aptusdt_tsmom_d1);

// APT-TSMOM-H12: 134d PF=1.34 180d PF=1.83 365d PF=1.83 730d PF=2.29
chimera::EdgeEngine::Config s38_aptusdt_tsmom_h12_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=20, .hold_bars=20, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_aptusdt_tsmom_h12(s38_aptusdt_tsmom_h12_cfg);
wire_engine(s38_aptusdt_tsmom_h12);

// APT-TSMOM-H1: 134d PF=1.53 180d PF=1.43 365d PF=1.40 730d PF=1.42
chimera::EdgeEngine::Config s38_aptusdt_tsmom_h1_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_aptusdt_tsmom_h1(s38_aptusdt_tsmom_h1_cfg);
wire_engine(s38_aptusdt_tsmom_h1);

// APT-TSMOM-H4: 134d PF=1.70 180d PF=1.72 365d PF=2.20 730d PF=2.15
chimera::EdgeEngine::Config s38_aptusdt_tsmom_h4_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=40, .hold_bars=6, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_aptusdt_tsmom_h4(s38_aptusdt_tsmom_h4_cfg);
wire_engine(s38_aptusdt_tsmom_h4);

// APT-ICHI-H4: 134d PF=2.38 180d PF=2.83 365d PF=1.86 730d PF=1.72
chimera::EdgeEngine::Config s38_aptusdt_ichi_h4_cfg{
    .symbol="aptusdt", .tag="APT-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=40, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_aptusdt_ichi_h4(s38_aptusdt_ichi_h4_cfg);
wire_engine(s38_aptusdt_ichi_h4);

// APT-BOPB-H4: 134d PF=2.09 180d PF=2.31 365d PF=1.90 730d PF=1.56
chimera::EdgeEngine::Config s38_aptusdt_bopb_h4_cfg{
    .symbol="aptusdt", .tag="APT-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=10, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s38_aptusdt_bopb_h4(s38_aptusdt_bopb_h4_cfg);
wire_engine(s38_aptusdt_bopb_h4);

// ─── PUSH_BACK BLOCKS ─────
g_slots.push_back({chimera::SYM_XRP, &s38_xrpusdt_tsmom_d1, "xrpusdt", 86400, "XRP-TSMOM-D1", 1.95, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_XRP, &s38_xrpusdt_ichi_h8, "xrpusdt", 28800, "XRP-ICHI-H8", 2.75, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_XRP, &s38_xrpusdt_ichi_h4, "xrpusdt", 14400, "XRP-ICHI-H4", 1.88, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_tsmom_d1, "wifusdt", 86400, "WIF-TSMOM-D1", 2.93, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_tsmom_h12, "wifusdt", 43200, "WIF-TSMOM-H12", 2.01, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_tsmom_h1, "wifusdt", 3600, "WIF-TSMOM-H1", 1.67, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_tsmom_h8, "wifusdt", 28800, "WIF-TSMOM-H8", 2.76, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_tsmom_h4, "wifusdt", 14400, "WIF-TSMOM-H4", 1.69, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_ichi_h1, "wifusdt", 3600, "WIF-ICHI-H1", 1.72, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_ichi_h8, "wifusdt", 28800, "WIF-ICHI-H8", 2.38, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_WIF, &s38_wifusdt_dt_h8, "wifusdt", 28800, "WIF-DT-H8", 1.60, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_TIA, &s38_tiausdt_tsmom_d1, "tiausdt", 86400, "TIA-TSMOM-D1", 1.98, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_TIA, &s38_tiausdt_tsmom_h1, "tiausdt", 3600, "TIA-TSMOM-H1", 1.61, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_TIA, &s38_tiausdt_tsmom_h4, "tiausdt", 14400, "TIA-TSMOM-H4", 2.19, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_TIA, &s38_tiausdt_ichi_h12, "tiausdt", 43200, "TIA-ICHI-H12", 1.35, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_TIA, &s38_tiausdt_ichi_h1, "tiausdt", 3600, "TIA-ICHI-H1", 1.70, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_TIA, &s38_tiausdt_ichi_h8, "tiausdt", 28800, "TIA-ICHI-H8", 1.37, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_TIA, &s38_tiausdt_ichi_h4, "tiausdt", 14400, "TIA-ICHI-H4", 2.04, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_tsmom_d1, "suiusdt", 86400, "SUI-TSMOM-D1", 2.72, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_tsmom_h12, "suiusdt", 43200, "SUI-TSMOM-H12", 1.80, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_tsmom_h1, "suiusdt", 3600, "SUI-TSMOM-H1", 1.59, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_ichi_h8, "suiusdt", 28800, "SUI-ICHI-H8", 2.59, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_ichi_h4, "suiusdt", 14400, "SUI-ICHI-H4", 1.73, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_dt_h12, "suiusdt", 43200, "SUI-DT-H12", 1.30, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_bopb_h1, "suiusdt", 3600, "SUI-BOPB-H1", 1.61, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SUI, &s38_suiusdt_bopb_h4, "suiusdt", 14400, "SUI-BOPB-H4", 1.74, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SOL, &s38_solusdt_tsmom_h8, "solusdt", 28800, "SOL-TSMOM-H8", 2.28, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SOL, &s38_solusdt_ichi_h12, "solusdt", 43200, "SOL-ICHI-H12", 1.96, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_SOL, &s38_solusdt_ichi_h4, "solusdt", 14400, "SOL-ICHI-H4", 2.23, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_tsmom_d1, "pepeusdt", 86400, "PEPE-TSMOM-D1", 2.32, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_tsmom_h12, "pepeusdt", 43200, "PEPE-TSMOM-H12", 1.98, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_tsmom_h8, "pepeusdt", 28800, "PEPE-TSMOM-H8", 2.67, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_tsmom_h4, "pepeusdt", 14400, "PEPE-TSMOM-H4", 1.93, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_sr_h12, "pepeusdt", 43200, "PEPE-SR-H12", 1.34, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_ichi_h12, "pepeusdt", 43200, "PEPE-ICHI-H12", 3.01, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_ichi_h8, "pepeusdt", 28800, "PEPE-ICHI-H8", 3.16, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_ichi_h4, "pepeusdt", 14400, "PEPE-ICHI-H4", 2.15, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_PEPE, &s38_pepeusdt_dt_h8, "pepeusdt", 28800, "PEPE-DT-H8", 1.38, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_tsmom_d1, "ondousdt", 86400, "ONDO-TSMOM-D1", 99.90, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_tsmom_h1, "ondousdt", 3600, "ONDO-TSMOM-H1", 1.85, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_tsmom_h8, "ondousdt", 28800, "ONDO-TSMOM-H8", 2.87, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_tsmom_h4, "ondousdt", 14400, "ONDO-TSMOM-H4", 2.08, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_ichi_h12, "ondousdt", 43200, "ONDO-ICHI-H12", 4.36, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_ichi_h8, "ondousdt", 28800, "ONDO-ICHI-H8", 1.52, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_ichi_h4, "ondousdt", 14400, "ONDO-ICHI-H4", 2.68, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ONDO, &s38_ondousdt_dt_h12, "ondousdt", 43200, "ONDO-DT-H12", 8.39, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_NEAR, &s38_nearusdt_tsmom_h1, "nearusdt", 3600, "NEAR-TSMOM-H1", 1.51, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_NEAR, &s38_nearusdt_ichi_h1, "nearusdt", 3600, "NEAR-ICHI-H1", 1.70, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_NEAR, &s38_nearusdt_ichi_h4, "nearusdt", 14400, "NEAR-ICHI-H4", 2.28, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_LINK, &s38_linkusdt_ichi_h12, "linkusdt", 43200, "LINK-ICHI-H12", 2.15, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_LINK, &s38_linkusdt_ichi_h4, "linkusdt", 14400, "LINK-ICHI-H4", 1.97, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_tsmom_h1, "fetusdt", 3600, "FET-TSMOM-H1", 1.41, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_tsmom_h4, "fetusdt", 14400, "FET-TSMOM-H4", 1.87, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_ichi_h12, "fetusdt", 43200, "FET-ICHI-H12", 1.64, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_ichi_h1, "fetusdt", 3600, "FET-ICHI-H1", 1.40, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_ichi_h8, "fetusdt", 28800, "FET-ICHI-H8", 1.71, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_ichi_h4, "fetusdt", 14400, "FET-ICHI-H4", 2.45, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_dch_h4, "fetusdt", 14400, "FET-DCH-H4", 1.57, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_FET, &s38_fetusdt_bopb_h4, "fetusdt", 14400, "FET-BOPB-H4", 1.74, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ETH, &s38_ethusdt_tsmom_h12, "ethusdt", 43200, "ETH-TSMOM-H12", 2.38, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ETH, &s38_ethusdt_ichi_h4, "ethusdt", 14400, "ETH-ICHI-H4", 2.41, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_DOGE, &s38_dogeusdt_tsmom_d1, "dogeusdt", 86400, "DOGE-TSMOM-D1", 2.90, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_DOGE, &s38_dogeusdt_tsmom_h4, "dogeusdt", 14400, "DOGE-TSMOM-H4", 1.99, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_DOGE, &s38_dogeusdt_ichi_h1, "dogeusdt", 3600, "DOGE-ICHI-H1", 1.51, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_DOGE, &s38_dogeusdt_ichi_h8, "dogeusdt", 28800, "DOGE-ICHI-H8", 2.19, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_DOGE, &s38_dogeusdt_ichi_h4, "dogeusdt", 14400, "DOGE-ICHI-H4", 2.01, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_DOGE, &s38_dogeusdt_bopb_h4, "dogeusdt", 14400, "DOGE-BOPB-H4", 1.85, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_BTC, &s38_btcusdt_ichi_h8, "btcusdt", 28800, "BTC-ICHI-H8", 1.67, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_BTC, &s38_btcusdt_ichi_h4, "btcusdt", 14400, "BTC-ICHI-H4", 2.18, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_BNB, &s38_bnbusdt_ichi_h12, "bnbusdt", 43200, "BNB-ICHI-H12", 1.62, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_BNB, &s38_bnbusdt_ichi_h8, "bnbusdt", 28800, "BNB-ICHI-H8", 1.88, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_BNB, &s38_bnbusdt_ichi_h4, "bnbusdt", 14400, "BNB-ICHI-H4", 1.58, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_AVAX, &s38_avaxusdt_tsmom_d1, "avaxusdt", 86400, "AVAX-TSMOM-D1", 3.10, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_AVAX, &s38_avaxusdt_tsmom_h12, "avaxusdt", 43200, "AVAX-TSMOM-H12", 1.79, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_AVAX, &s38_avaxusdt_ichi_h8, "avaxusdt", 28800, "AVAX-ICHI-H8", 2.12, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_AVAX, &s38_avaxusdt_ichi_h4, "avaxusdt", 14400, "AVAX-ICHI-H4", 1.93, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_tsmom_d1, "arbusdt", 86400, "ARB-TSMOM-D1", 3.03, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_tsmom_h12, "arbusdt", 43200, "ARB-TSMOM-H12", 2.15, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_tsmom_h1, "arbusdt", 3600, "ARB-TSMOM-H1", 1.45, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_tsmom_h4, "arbusdt", 14400, "ARB-TSMOM-H4", 2.16, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_ichi_h12, "arbusdt", 43200, "ARB-ICHI-H12", 1.38, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_ichi_h1, "arbusdt", 3600, "ARB-ICHI-H1", 1.40, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_ichi_h8, "arbusdt", 28800, "ARB-ICHI-H8", 2.11, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_ichi_h4, "arbusdt", 14400, "ARB-ICHI-H4", 2.12, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_ARB, &s38_arbusdt_dch_h4, "arbusdt", 14400, "ARB-DCH-H4", 1.79, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_APT, &s38_aptusdt_tsmom_d1, "aptusdt", 86400, "APT-TSMOM-D1", 2.33, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_APT, &s38_aptusdt_tsmom_h12, "aptusdt", 43200, "APT-TSMOM-H12", 2.29, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_APT, &s38_aptusdt_tsmom_h1, "aptusdt", 3600, "APT-TSMOM-H1", 1.42, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_APT, &s38_aptusdt_tsmom_h4, "aptusdt", 14400, "APT-TSMOM-H4", 2.15, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_APT, &s38_aptusdt_ichi_h4, "aptusdt", 14400, "APT-ICHI-H4", 1.72, 0.0, 100, 730, 38});
g_slots.push_back({chimera::SYM_APT, &s38_aptusdt_bopb_h4, "aptusdt", 14400, "APT-BOPB-H4", 1.56, 0.0, 100, 730, 38});
