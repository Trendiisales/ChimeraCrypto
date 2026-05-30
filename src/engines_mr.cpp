// S53: mean-reversion sleeve — strict-validated spot-long dip-buyers
// (Bollinger lower-band + RSI oversold-bounce). Profit in chop/dips where
// momentum bleeds. fine-fill+regime-gate+730+4WF+crash+SL-perturbation.

// APT-BOLL20k20-H1
chimera::EdgeEngine::Config mr_apt_boll20k20_h1_cfg{
    .symbol="aptusdt", .tag="APT-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_apt_boll20k20_h1(mr_apt_boll20k20_h1_cfg);
wire_engine(mr_apt_boll20k20_h1);

// APT-BOLL20k25-H1
chimera::EdgeEngine::Config mr_apt_boll20k25_h1_cfg{
    .symbol="aptusdt", .tag="APT-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_apt_boll20k25_h1(mr_apt_boll20k25_h1_cfg);
wire_engine(mr_apt_boll20k25_h1);

// APT-BOLL30k20-H2
chimera::EdgeEngine::Config mr_apt_boll30k20_h2_cfg{
    .symbol="aptusdt", .tag="APT-BOLL30k20-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=30, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_apt_boll30k20_h2(mr_apt_boll30k20_h2_cfg);
wire_engine(mr_apt_boll30k20_h2);

// ARB-BOLL20k20-H1
chimera::EdgeEngine::Config mr_arb_boll20k20_h1_cfg{
    .symbol="arbusdt", .tag="ARB-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_arb_boll20k20_h1(mr_arb_boll20k20_h1_cfg);
wire_engine(mr_arb_boll20k20_h1);

// ARB-BOLL20k20-H2
chimera::EdgeEngine::Config mr_arb_boll20k20_h2_cfg{
    .symbol="arbusdt", .tag="ARB-BOLL20k20-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_arb_boll20k20_h2(mr_arb_boll20k20_h2_cfg);
wire_engine(mr_arb_boll20k20_h2);

// ARB-BOLL20k25-H1
chimera::EdgeEngine::Config mr_arb_boll20k25_h1_cfg{
    .symbol="arbusdt", .tag="ARB-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_arb_boll20k25_h1(mr_arb_boll20k25_h1_cfg);
wire_engine(mr_arb_boll20k25_h1);

// ARB-RSIR14t35-H1
chimera::EdgeEngine::Config mr_arb_rsir14t35_h1_cfg{
    .symbol="arbusdt", .tag="ARB-RSIR14t35-H1", .kind=chimera::StrategyKind::RSI_REVERT,
    .tf_secs=3600, .lookback=20, .hold_bars=20, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=35, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_arb_rsir14t35_h1(mr_arb_rsir14t35_h1_cfg);
wire_engine(mr_arb_rsir14t35_h1);

// ATOM-BOLL30k25-H1
chimera::EdgeEngine::Config mr_atom_boll30k25_h1_cfg{
    .symbol="atomusdt", .tag="ATOM-BOLL30k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=30, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_atom_boll30k25_h1(mr_atom_boll30k25_h1_cfg);
wire_engine(mr_atom_boll30k25_h1);

// DOGE-BOLL30k20-H2
chimera::EdgeEngine::Config mr_doge_boll30k20_h2_cfg{
    .symbol="dogeusdt", .tag="DOGE-BOLL30k20-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=30, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_doge_boll30k20_h2(mr_doge_boll30k20_h2_cfg);
wire_engine(mr_doge_boll30k20_h2);

// FET-BOLL20k20-H1
chimera::EdgeEngine::Config mr_fet_boll20k20_h1_cfg{
    .symbol="fetusdt", .tag="FET-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_fet_boll20k20_h1(mr_fet_boll20k20_h1_cfg);
wire_engine(mr_fet_boll20k20_h1);

// FET-BOLL20k25-H1
chimera::EdgeEngine::Config mr_fet_boll20k25_h1_cfg{
    .symbol="fetusdt", .tag="FET-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_fet_boll20k25_h1(mr_fet_boll20k25_h1_cfg);
wire_engine(mr_fet_boll20k25_h1);

// INJ-BOLL20k20-H1
chimera::EdgeEngine::Config mr_inj_boll20k20_h1_cfg{
    .symbol="injusdt", .tag="INJ-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_inj_boll20k20_h1(mr_inj_boll20k20_h1_cfg);
wire_engine(mr_inj_boll20k20_h1);

// INJ-BOLL20k25-H1
chimera::EdgeEngine::Config mr_inj_boll20k25_h1_cfg{
    .symbol="injusdt", .tag="INJ-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_inj_boll20k25_h1(mr_inj_boll20k25_h1_cfg);
wire_engine(mr_inj_boll20k25_h1);

// LDO-BOLL20k20-H1
chimera::EdgeEngine::Config mr_ldo_boll20k20_h1_cfg{
    .symbol="ldousdt", .tag="LDO-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_ldo_boll20k20_h1(mr_ldo_boll20k20_h1_cfg);
wire_engine(mr_ldo_boll20k20_h1);

// LDO-RSIR14t35-H1
chimera::EdgeEngine::Config mr_ldo_rsir14t35_h1_cfg{
    .symbol="ldousdt", .tag="LDO-RSIR14t35-H1", .kind=chimera::StrategyKind::RSI_REVERT,
    .tf_secs=3600, .lookback=20, .hold_bars=20, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=35, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_ldo_rsir14t35_h1(mr_ldo_rsir14t35_h1_cfg);
wire_engine(mr_ldo_rsir14t35_h1);

// LINK-BOLL30k20-H1
chimera::EdgeEngine::Config mr_link_boll30k20_h1_cfg{
    .symbol="linkusdt", .tag="LINK-BOLL30k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=30, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_link_boll30k20_h1(mr_link_boll30k20_h1_cfg);
wire_engine(mr_link_boll30k20_h1);

// NEAR-BOLL20k20-H1
chimera::EdgeEngine::Config mr_near_boll20k20_h1_cfg{
    .symbol="nearusdt", .tag="NEAR-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_near_boll20k20_h1(mr_near_boll20k20_h1_cfg);
wire_engine(mr_near_boll20k20_h1);

// NEAR-BOLL20k25-H1
chimera::EdgeEngine::Config mr_near_boll20k25_h1_cfg{
    .symbol="nearusdt", .tag="NEAR-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_near_boll20k25_h1(mr_near_boll20k25_h1_cfg);
wire_engine(mr_near_boll20k25_h1);

// OP-BOLL20k20-H1
chimera::EdgeEngine::Config mr_op_boll20k20_h1_cfg{
    .symbol="opusdt", .tag="OP-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_op_boll20k20_h1(mr_op_boll20k20_h1_cfg);
wire_engine(mr_op_boll20k20_h1);

// OP-BOLL20k25-H1
chimera::EdgeEngine::Config mr_op_boll20k25_h1_cfg{
    .symbol="opusdt", .tag="OP-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_op_boll20k25_h1(mr_op_boll20k25_h1_cfg);
wire_engine(mr_op_boll20k25_h1);

// OP-RSIR14t35-H1
chimera::EdgeEngine::Config mr_op_rsir14t35_h1_cfg{
    .symbol="opusdt", .tag="OP-RSIR14t35-H1", .kind=chimera::StrategyKind::RSI_REVERT,
    .tf_secs=3600, .lookback=20, .hold_bars=20, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=35, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_op_rsir14t35_h1(mr_op_rsir14t35_h1_cfg);
wire_engine(mr_op_rsir14t35_h1);

// PEPE-BOLL20k20-H1
chimera::EdgeEngine::Config mr_pepe_boll20k20_h1_cfg{
    .symbol="pepeusdt", .tag="PEPE-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_pepe_boll20k20_h1(mr_pepe_boll20k20_h1_cfg);
wire_engine(mr_pepe_boll20k20_h1);

// PEPE-BOLL20k25-H1
chimera::EdgeEngine::Config mr_pepe_boll20k25_h1_cfg{
    .symbol="pepeusdt", .tag="PEPE-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_pepe_boll20k25_h1(mr_pepe_boll20k25_h1_cfg);
wire_engine(mr_pepe_boll20k25_h1);

// PEPE-RSIR14t35-H1
chimera::EdgeEngine::Config mr_pepe_rsir14t35_h1_cfg{
    .symbol="pepeusdt", .tag="PEPE-RSIR14t35-H1", .kind=chimera::StrategyKind::RSI_REVERT,
    .tf_secs=3600, .lookback=20, .hold_bars=20, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=35, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_pepe_rsir14t35_h1(mr_pepe_rsir14t35_h1_cfg);
wire_engine(mr_pepe_rsir14t35_h1);

// SEI-BOLL30k20-H1
chimera::EdgeEngine::Config mr_sei_boll30k20_h1_cfg{
    .symbol="seiusdt", .tag="SEI-BOLL30k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=30, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_sei_boll30k20_h1(mr_sei_boll30k20_h1_cfg);
wire_engine(mr_sei_boll30k20_h1);

// SUI-BOLL20k20-H1
chimera::EdgeEngine::Config mr_sui_boll20k20_h1_cfg{
    .symbol="suiusdt", .tag="SUI-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_sui_boll20k20_h1(mr_sui_boll20k20_h1_cfg);
wire_engine(mr_sui_boll20k20_h1);

// TIA-BOLL20k20-H1
chimera::EdgeEngine::Config mr_tia_boll20k20_h1_cfg{
    .symbol="tiausdt", .tag="TIA-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_tia_boll20k20_h1(mr_tia_boll20k20_h1_cfg);
wire_engine(mr_tia_boll20k20_h1);

// TIA-BOLL30k25-H1
chimera::EdgeEngine::Config mr_tia_boll30k25_h1_cfg{
    .symbol="tiausdt", .tag="TIA-BOLL30k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=30, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_tia_boll30k25_h1(mr_tia_boll30k25_h1_cfg);
wire_engine(mr_tia_boll30k25_h1);

// UNI-BOLL20k20-H1
chimera::EdgeEngine::Config mr_uni_boll20k20_h1_cfg{
    .symbol="uniusdt", .tag="UNI-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_uni_boll20k20_h1(mr_uni_boll20k20_h1_cfg);
wire_engine(mr_uni_boll20k20_h1);

// WIF-BOLL20k20-H1
chimera::EdgeEngine::Config mr_wif_boll20k20_h1_cfg{
    .symbol="wifusdt", .tag="WIF-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_wif_boll20k20_h1(mr_wif_boll20k20_h1_cfg);
wire_engine(mr_wif_boll20k20_h1);

// WIF-BOLL20k25-H1
chimera::EdgeEngine::Config mr_wif_boll20k25_h1_cfg{
    .symbol="wifusdt", .tag="WIF-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_wif_boll20k25_h1(mr_wif_boll20k25_h1_cfg);
wire_engine(mr_wif_boll20k25_h1);

// ZRO-BOLL20k20-H1
chimera::EdgeEngine::Config mr_zro_boll20k20_h1_cfg{
    .symbol="zrousdt", .tag="ZRO-BOLL20k20-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_zro_boll20k20_h1(mr_zro_boll20k20_h1_cfg);
wire_engine(mr_zro_boll20k20_h1);

// ZRO-BOLL20k25-H1
chimera::EdgeEngine::Config mr_zro_boll20k25_h1_cfg{
    .symbol="zrousdt", .tag="ZRO-BOLL20k25-H1", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=3600, .lookback=20, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.5, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_zro_boll20k25_h1(mr_zro_boll20k25_h1_cfg);
wire_engine(mr_zro_boll20k25_h1);

// ZRO-BOLL30k20-H2
chimera::EdgeEngine::Config mr_zro_boll30k20_h2_cfg{
    .symbol="zrousdt", .tag="ZRO-BOLL30k20-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=30, .hold_bars=15, .sl_atr_mult=2.5, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine mr_zro_boll30k20_h2(mr_zro_boll30k20_h2_cfg);
wire_engine(mr_zro_boll30k20_h2);

