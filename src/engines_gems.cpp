// S52: salvaged gems from s41/s42 — strict-validated (fine-fill+regime-gate,
// 365+730+4WF+crash+SL-perturbation). Re-compiled into the live roster.

// TURBO-TSMOM-H4 (s42)
chimera::EdgeEngine::Config s42_turbousdt_tsmom_h4_cfg{
    .symbol="turbousdt", .tag="TURBO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_turbousdt_tsmom_h4(s42_turbousdt_tsmom_h4_cfg);
wire_engine(s42_turbousdt_tsmom_h4);

// ETHFI-TSMOM-H4 (s42)
chimera::EdgeEngine::Config s42_ethfiusdt_tsmom_h4_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=5, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ethfiusdt_tsmom_h4(s42_ethfiusdt_tsmom_h4_cfg);
wire_engine(s42_ethfiusdt_tsmom_h4);

// ZRO-ICHI-H3 (s42)
chimera::EdgeEngine::Config s42_zrousdt_ichi_h3_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_ichi_h3(s42_zrousdt_ichi_h3_cfg);
wire_engine(s42_zrousdt_ichi_h3);

// SUI-BOPB-H1 (s41)
chimera::EdgeEngine::Config s41_suiusdt_bopb_h1_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H1", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=3600, .lookback=40, .hold_bars=20, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_suiusdt_bopb_h1(s41_suiusdt_bopb_h1_cfg);
wire_engine(s41_suiusdt_bopb_h1);

// FET-TSMOM-H3 (s41)
chimera::EdgeEngine::Config s41_fetusdt_tsmom_h3_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_fetusdt_tsmom_h3(s41_fetusdt_tsmom_h3_cfg);
wire_engine(s41_fetusdt_tsmom_h3);

// PEPE-TSMOM-H8 (s41)
chimera::EdgeEngine::Config s41_pepeusdt_tsmom_h8_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_pepeusdt_tsmom_h8(s41_pepeusdt_tsmom_h8_cfg);
wire_engine(s41_pepeusdt_tsmom_h8);

// FLOKI-ICHI-H8 (s42)
chimera::EdgeEngine::Config s42_flokiusdt_ichi_h8_cfg{
    .symbol="flokiusdt", .tag="FLOKI-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=18, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_ichi_h8(s42_flokiusdt_ichi_h8_cfg);
wire_engine(s42_flokiusdt_ichi_h8);

// ONDO-TSMOM-D1 (s42)
chimera::EdgeEngine::Config s42_ondousdt_tsmom_d1_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_tsmom_d1(s42_ondousdt_tsmom_d1_cfg);
wire_engine(s42_ondousdt_tsmom_d1);

// UNI-BOLL-H2 (s42)
chimera::EdgeEngine::Config s42_uniusdt_boll_h2_cfg{
    .symbol="uniusdt", .tag="UNI-BOLL-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=18, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_uniusdt_boll_h2(s42_uniusdt_boll_h2_cfg);
wire_engine(s42_uniusdt_boll_h2);

// ONDO-TSMOM-H8 (s41)
chimera::EdgeEngine::Config s41_ondousdt_tsmom_h8_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s41_ondousdt_tsmom_h8(s41_ondousdt_tsmom_h8_cfg);
wire_engine(s41_ondousdt_tsmom_h8);

// ETHFI-TSMOM-H12 (s42)
chimera::EdgeEngine::Config s42_ethfiusdt_tsmom_h12_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ethfiusdt_tsmom_h12(s42_ethfiusdt_tsmom_h12_cfg);
wire_engine(s42_ethfiusdt_tsmom_h12);

// SUI-TSMOM-D1 (s42)
chimera::EdgeEngine::Config s42_suiusdt_tsmom_d1_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_suiusdt_tsmom_d1(s42_suiusdt_tsmom_d1_cfg);
wire_engine(s42_suiusdt_tsmom_d1);

// FLOKI-TSMOM-D1 (s42)
chimera::EdgeEngine::Config s42_flokiusdt_tsmom_d1_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_tsmom_d1(s42_flokiusdt_tsmom_d1_cfg);
wire_engine(s42_flokiusdt_tsmom_d1);

