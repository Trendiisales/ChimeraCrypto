// S42 ADDS — 213 net new engines, PF>=2.0 across all 4 WF windows
// 62-sym × 10-TF × 144 grid × pyramid_xlow. Delisted MATIC/MKR dropped.
// Top: JTO-TSMOM-H4 60/3/1.0 +248k bp, BOME-TSMOM-H4 +218k, MKR dropped.
// ─── CONFIG + ENGINE + WIRE ─────────────────────────────────────────────
// JTO-TSMOM-H4 minPF=2.41 score=248121
chimera::EdgeEngine::Config s42_jtousdt_tsmom_h4_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_tsmom_h4(s42_jtousdt_tsmom_h4_cfg);
wire_engine(s42_jtousdt_tsmom_h4);

// JTO-TSMOM-H3 minPF=2.25 score=232905
chimera::EdgeEngine::Config s42_jtousdt_tsmom_h3_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=18, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_tsmom_h3(s42_jtousdt_tsmom_h3_cfg);
wire_engine(s42_jtousdt_tsmom_h3);

// JTO-TSMOM-H8 minPF=2.71 score=222662
chimera::EdgeEngine::Config s42_jtousdt_tsmom_h8_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_tsmom_h8(s42_jtousdt_tsmom_h8_cfg);
wire_engine(s42_jtousdt_tsmom_h8);

// BOME-TSMOM-H4 minPF=2.10 score=217727
chimera::EdgeEngine::Config s42_bomeusdt_tsmom_h4_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bomeusdt_tsmom_h4(s42_bomeusdt_tsmom_h4_cfg);
wire_engine(s42_bomeusdt_tsmom_h4);

// BOME-TSMOM-H3 minPF=2.00 score=211262
chimera::EdgeEngine::Config s42_bomeusdt_tsmom_h3_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bomeusdt_tsmom_h3(s42_bomeusdt_tsmom_h3_cfg);
wire_engine(s42_bomeusdt_tsmom_h3);

// JTO-TSMOM-H12 minPF=2.46 score=208417
chimera::EdgeEngine::Config s42_jtousdt_tsmom_h12_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_tsmom_h12(s42_jtousdt_tsmom_h12_cfg);
wire_engine(s42_jtousdt_tsmom_h12);

// INJ-TSMOM-H3 minPF=2.10 score=199704
chimera::EdgeEngine::Config s42_injusdt_tsmom_h3_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_injusdt_tsmom_h3(s42_injusdt_tsmom_h3_cfg);
wire_engine(s42_injusdt_tsmom_h3);

// FLOKI-TSMOM-H3 minPF=2.01 score=192860
chimera::EdgeEngine::Config s42_flokiusdt_tsmom_h3_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_tsmom_h3(s42_flokiusdt_tsmom_h3_cfg);
wire_engine(s42_flokiusdt_tsmom_h3);

// JTO-TSMOM-H6 minPF=2.24 score=192076
chimera::EdgeEngine::Config s42_jtousdt_tsmom_h6_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_tsmom_h6(s42_jtousdt_tsmom_h6_cfg);
wire_engine(s42_jtousdt_tsmom_h6);

// INJ-TSMOM-H4 minPF=2.25 score=185680
chimera::EdgeEngine::Config s42_injusdt_tsmom_h4_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_injusdt_tsmom_h4(s42_injusdt_tsmom_h4_cfg);
wire_engine(s42_injusdt_tsmom_h4);

// ENA-TSMOM-H3 minPF=2.16 score=183466
chimera::EdgeEngine::Config s42_enausdt_tsmom_h3_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_enausdt_tsmom_h3(s42_enausdt_tsmom_h3_cfg);
wire_engine(s42_enausdt_tsmom_h3);

// BOME-TSMOM-H6 minPF=2.16 score=172404
chimera::EdgeEngine::Config s42_bomeusdt_tsmom_h6_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bomeusdt_tsmom_h6(s42_bomeusdt_tsmom_h6_cfg);
wire_engine(s42_bomeusdt_tsmom_h6);

// COMP-TSMOM-H3 minPF=2.07 score=170590
chimera::EdgeEngine::Config s42_compusdt_tsmom_h3_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_compusdt_tsmom_h3(s42_compusdt_tsmom_h3_cfg);
wire_engine(s42_compusdt_tsmom_h3);

// ENA-TSMOM-H8 minPF=2.48 score=168160
chimera::EdgeEngine::Config s42_enausdt_tsmom_h8_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_enausdt_tsmom_h8(s42_enausdt_tsmom_h8_cfg);
wire_engine(s42_enausdt_tsmom_h8);

// ICP-TSMOM-H4 minPF=2.41 score=167999
chimera::EdgeEngine::Config s42_icpusdt_tsmom_h4_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_icpusdt_tsmom_h4(s42_icpusdt_tsmom_h4_cfg);
wire_engine(s42_icpusdt_tsmom_h4);

// TIA-TSMOM-H4 minPF=2.00 score=166613
chimera::EdgeEngine::Config s42_tiausdt_tsmom_h4_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_tsmom_h4(s42_tiausdt_tsmom_h4_cfg);
wire_engine(s42_tiausdt_tsmom_h4);

// WIF-TSMOM-H6 minPF=2.22 score=165662
chimera::EdgeEngine::Config s42_wifusdt_tsmom_h6_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_wifusdt_tsmom_h6(s42_wifusdt_tsmom_h6_cfg);
wire_engine(s42_wifusdt_tsmom_h6);

// COMP-TSMOM-H4 minPF=2.14 score=165463
chimera::EdgeEngine::Config s42_compusdt_tsmom_h4_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_compusdt_tsmom_h4(s42_compusdt_tsmom_h4_cfg);
wire_engine(s42_compusdt_tsmom_h4);

// FET-TSMOM-H6 minPF=2.90 score=164947
chimera::EdgeEngine::Config s42_fetusdt_tsmom_h6_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_fetusdt_tsmom_h6(s42_fetusdt_tsmom_h6_cfg);
wire_engine(s42_fetusdt_tsmom_h6);

// FLOKI-TSMOM-H4 minPF=2.21 score=164029
chimera::EdgeEngine::Config s42_flokiusdt_tsmom_h4_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_tsmom_h4(s42_flokiusdt_tsmom_h4_cfg);
wire_engine(s42_flokiusdt_tsmom_h4);

// ENA-TSMOM-H6 minPF=2.27 score=162618
chimera::EdgeEngine::Config s42_enausdt_tsmom_h6_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_enausdt_tsmom_h6(s42_enausdt_tsmom_h6_cfg);
wire_engine(s42_enausdt_tsmom_h6);

// BOME-TSMOM-H8 minPF=2.26 score=162267
chimera::EdgeEngine::Config s42_bomeusdt_tsmom_h8_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bomeusdt_tsmom_h8(s42_bomeusdt_tsmom_h8_cfg);
wire_engine(s42_bomeusdt_tsmom_h8);

// INJ-TSMOM-H6 minPF=2.42 score=159728
chimera::EdgeEngine::Config s42_injusdt_tsmom_h6_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_injusdt_tsmom_h6(s42_injusdt_tsmom_h6_cfg);
wire_engine(s42_injusdt_tsmom_h6);

// JUP-TSMOM-H4 minPF=2.01 score=158080
chimera::EdgeEngine::Config s42_jupusdt_tsmom_h4_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jupusdt_tsmom_h4(s42_jupusdt_tsmom_h4_cfg);
wire_engine(s42_jupusdt_tsmom_h4);

// UNI-TSMOM-H4 minPF=2.17 score=156889
chimera::EdgeEngine::Config s42_uniusdt_tsmom_h4_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_uniusdt_tsmom_h4(s42_uniusdt_tsmom_h4_cfg);
wire_engine(s42_uniusdt_tsmom_h4);

// PEPE-TSMOM-H4 minPF=2.11 score=152731
chimera::EdgeEngine::Config s42_pepeusdt_tsmom_h4_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_pepeusdt_tsmom_h4(s42_pepeusdt_tsmom_h4_cfg);
wire_engine(s42_pepeusdt_tsmom_h4);

// WIF-TSMOM-H8 minPF=2.09 score=150842
chimera::EdgeEngine::Config s42_wifusdt_tsmom_h8_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_wifusdt_tsmom_h8(s42_wifusdt_tsmom_h8_cfg);
wire_engine(s42_wifusdt_tsmom_h8);

// IMX-TSMOM-H4 minPF=2.34 score=150505
chimera::EdgeEngine::Config s42_imxusdt_tsmom_h4_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_imxusdt_tsmom_h4(s42_imxusdt_tsmom_h4_cfg);
wire_engine(s42_imxusdt_tsmom_h4);

// TIA-TSMOM-H6 minPF=2.06 score=149745
chimera::EdgeEngine::Config s42_tiausdt_tsmom_h6_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_tsmom_h6(s42_tiausdt_tsmom_h6_cfg);
wire_engine(s42_tiausdt_tsmom_h6);

// COMP-TSMOM-H6 minPF=2.35 score=148181
chimera::EdgeEngine::Config s42_compusdt_tsmom_h6_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_compusdt_tsmom_h6(s42_compusdt_tsmom_h6_cfg);
wire_engine(s42_compusdt_tsmom_h6);

// ETHFI-TSMOM-H4 minPF=2.05 score=146388
chimera::EdgeEngine::Config s42_ethfiusdt_tsmom_h4_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=5, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ethfiusdt_tsmom_h4(s42_ethfiusdt_tsmom_h4_cfg);
wire_engine(s42_ethfiusdt_tsmom_h4);

// FET-TSMOM-H4 minPF=2.29 score=146028
chimera::EdgeEngine::Config s42_fetusdt_tsmom_h4_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_fetusdt_tsmom_h4(s42_fetusdt_tsmom_h4_cfg);
wire_engine(s42_fetusdt_tsmom_h4);

// FLOKI-TSMOM-H6 minPF=2.04 score=145740
chimera::EdgeEngine::Config s42_flokiusdt_tsmom_h6_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_tsmom_h6(s42_flokiusdt_tsmom_h6_cfg);
wire_engine(s42_flokiusdt_tsmom_h6);

// ICP-TSMOM-H8 minPF=2.43 score=145083
chimera::EdgeEngine::Config s42_icpusdt_tsmom_h8_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_icpusdt_tsmom_h8(s42_icpusdt_tsmom_h8_cfg);
wire_engine(s42_icpusdt_tsmom_h8);

// WIF-TSMOM-H12 minPF=2.10 score=143832
chimera::EdgeEngine::Config s42_wifusdt_tsmom_h12_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_wifusdt_tsmom_h12(s42_wifusdt_tsmom_h12_cfg);
wire_engine(s42_wifusdt_tsmom_h12);

// SUI-TSMOM-H3 minPF=2.13 score=142297
chimera::EdgeEngine::Config s42_suiusdt_tsmom_h3_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_suiusdt_tsmom_h3(s42_suiusdt_tsmom_h3_cfg);
wire_engine(s42_suiusdt_tsmom_h3);

// ICP-TSMOM-H6 minPF=2.20 score=142219
chimera::EdgeEngine::Config s42_icpusdt_tsmom_h6_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_icpusdt_tsmom_h6(s42_icpusdt_tsmom_h6_cfg);
wire_engine(s42_icpusdt_tsmom_h6);

// UNI-TSMOM-H6 minPF=2.45 score=141274
chimera::EdgeEngine::Config s42_uniusdt_tsmom_h6_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_uniusdt_tsmom_h6(s42_uniusdt_tsmom_h6_cfg);
wire_engine(s42_uniusdt_tsmom_h6);

// BOME-TSMOM-D1 minPF=2.62 score=140367
chimera::EdgeEngine::Config s42_bomeusdt_tsmom_d1_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bomeusdt_tsmom_d1(s42_bomeusdt_tsmom_d1_cfg);
wire_engine(s42_bomeusdt_tsmom_d1);

// SEI-TSMOM-H4 minPF=2.11 score=139728
chimera::EdgeEngine::Config s42_seiusdt_tsmom_h4_cfg{
    .symbol="seiusdt", .tag="SEI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_seiusdt_tsmom_h4(s42_seiusdt_tsmom_h4_cfg);
wire_engine(s42_seiusdt_tsmom_h4);

// TIA-TSMOM-H3 minPF=2.15 score=139378
chimera::EdgeEngine::Config s42_tiausdt_tsmom_h3_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_tsmom_h3(s42_tiausdt_tsmom_h3_cfg);
wire_engine(s42_tiausdt_tsmom_h3);

// OP-TSMOM-H2 minPF=2.04 score=132799
chimera::EdgeEngine::Config s42_opusdt_tsmom_h2_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_opusdt_tsmom_h2(s42_opusdt_tsmom_h2_cfg);
wire_engine(s42_opusdt_tsmom_h2);

// IMX-TSMOM-H8 minPF=2.61 score=132754
chimera::EdgeEngine::Config s42_imxusdt_tsmom_h8_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_imxusdt_tsmom_h8(s42_imxusdt_tsmom_h8_cfg);
wire_engine(s42_imxusdt_tsmom_h8);

// LDO-TSMOM-H12 minPF=2.72 score=132114
chimera::EdgeEngine::Config s42_ldousdt_tsmom_h12_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ldousdt_tsmom_h12(s42_ldousdt_tsmom_h12_cfg);
wire_engine(s42_ldousdt_tsmom_h12);

// ARB-TSMOM-H4 minPF=2.16 score=131386
chimera::EdgeEngine::Config s42_arbusdt_tsmom_h4_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_arbusdt_tsmom_h4(s42_arbusdt_tsmom_h4_cfg);
wire_engine(s42_arbusdt_tsmom_h4);

// COMP-TSMOM-H8 minPF=2.43 score=130180
chimera::EdgeEngine::Config s42_compusdt_tsmom_h8_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_compusdt_tsmom_h8(s42_compusdt_tsmom_h8_cfg);
wire_engine(s42_compusdt_tsmom_h8);

// ZRO-TSMOM-H3 minPF=2.28 score=129747
chimera::EdgeEngine::Config s42_zrousdt_tsmom_h3_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_tsmom_h3(s42_zrousdt_tsmom_h3_cfg);
wire_engine(s42_zrousdt_tsmom_h3);

// PYTH-TSMOM-H6 minPF=2.09 score=129621
chimera::EdgeEngine::Config s42_pythusdt_tsmom_h6_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_pythusdt_tsmom_h6(s42_pythusdt_tsmom_h6_cfg);
wire_engine(s42_pythusdt_tsmom_h6);

// ENA-ICHI-H2 minPF=2.00 score=128237
chimera::EdgeEngine::Config s42_enausdt_ichi_h2_cfg{
    .symbol="enausdt", .tag="ENA-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_enausdt_ichi_h2(s42_enausdt_ichi_h2_cfg);
wire_engine(s42_enausdt_ichi_h2);

// UNI-TSMOM-H12 minPF=2.72 score=127728
chimera::EdgeEngine::Config s42_uniusdt_tsmom_h12_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_uniusdt_tsmom_h12(s42_uniusdt_tsmom_h12_cfg);
wire_engine(s42_uniusdt_tsmom_h12);

// STX-TSMOM-H6 minPF=2.05 score=126479
chimera::EdgeEngine::Config s42_stxusdt_tsmom_h6_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_stxusdt_tsmom_h6(s42_stxusdt_tsmom_h6_cfg);
wire_engine(s42_stxusdt_tsmom_h6);

// ENA-TSMOM-H12 minPF=2.36 score=126129
chimera::EdgeEngine::Config s42_enausdt_tsmom_h12_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_enausdt_tsmom_h12(s42_enausdt_tsmom_h12_cfg);
wire_engine(s42_enausdt_tsmom_h12);

// BOME-TSMOM-H12 minPF=2.36 score=124776
chimera::EdgeEngine::Config s42_bomeusdt_tsmom_h12_cfg{
    .symbol="bomeusdt", .tag="BOME-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bomeusdt_tsmom_h12(s42_bomeusdt_tsmom_h12_cfg);
wire_engine(s42_bomeusdt_tsmom_h12);

// FLOKI-TSMOM-H8 minPF=2.21 score=124240
chimera::EdgeEngine::Config s42_flokiusdt_tsmom_h8_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_tsmom_h8(s42_flokiusdt_tsmom_h8_cfg);
wire_engine(s42_flokiusdt_tsmom_h8);

// LDO-TSMOM-H8 minPF=2.15 score=123471
chimera::EdgeEngine::Config s42_ldousdt_tsmom_h8_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ldousdt_tsmom_h8(s42_ldousdt_tsmom_h8_cfg);
wire_engine(s42_ldousdt_tsmom_h8);

// UNI-TSMOM-H8 minPF=2.30 score=122060
chimera::EdgeEngine::Config s42_uniusdt_tsmom_h8_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_uniusdt_tsmom_h8(s42_uniusdt_tsmom_h8_cfg);
wire_engine(s42_uniusdt_tsmom_h8);

// ARKM-TSMOM-H12 minPF=2.25 score=120386
chimera::EdgeEngine::Config s42_arkmusdt_tsmom_h12_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_arkmusdt_tsmom_h12(s42_arkmusdt_tsmom_h12_cfg);
wire_engine(s42_arkmusdt_tsmom_h12);

// STX-TSMOM-H8 minPF=2.44 score=119831
chimera::EdgeEngine::Config s42_stxusdt_tsmom_h8_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_stxusdt_tsmom_h8(s42_stxusdt_tsmom_h8_cfg);
wire_engine(s42_stxusdt_tsmom_h8);

// ZRO-TSMOM-H4 minPF=2.72 score=118998
chimera::EdgeEngine::Config s42_zrousdt_tsmom_h4_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_tsmom_h4(s42_zrousdt_tsmom_h4_cfg);
wire_engine(s42_zrousdt_tsmom_h4);

// JUP-TSMOM-H12 minPF=2.35 score=118092
chimera::EdgeEngine::Config s42_jupusdt_tsmom_h12_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jupusdt_tsmom_h12(s42_jupusdt_tsmom_h12_cfg);
wire_engine(s42_jupusdt_tsmom_h12);

// PEPE-TSMOM-H12 minPF=2.03 score=117296
chimera::EdgeEngine::Config s42_pepeusdt_tsmom_h12_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_pepeusdt_tsmom_h12(s42_pepeusdt_tsmom_h12_cfg);
wire_engine(s42_pepeusdt_tsmom_h12);

// TURBO-TSMOM-H4 minPF=2.07 score=116648
chimera::EdgeEngine::Config s42_turbousdt_tsmom_h4_cfg{
    .symbol="turbousdt", .tag="TURBO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_turbousdt_tsmom_h4(s42_turbousdt_tsmom_h4_cfg);
wire_engine(s42_turbousdt_tsmom_h4);

// SUI-ICHI-H3 minPF=2.23 score=116440
chimera::EdgeEngine::Config s42_suiusdt_ichi_h3_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_suiusdt_ichi_h3(s42_suiusdt_ichi_h3_cfg);
wire_engine(s42_suiusdt_ichi_h3);

// AVAX-TSMOM-H3 minPF=2.00 score=115188
chimera::EdgeEngine::Config s42_avaxusdt_tsmom_h3_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_avaxusdt_tsmom_h3(s42_avaxusdt_tsmom_h3_cfg);
wire_engine(s42_avaxusdt_tsmom_h3);

// COMP-TSMOM-H12 minPF=2.10 score=111111
chimera::EdgeEngine::Config s42_compusdt_tsmom_h12_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_compusdt_tsmom_h12(s42_compusdt_tsmom_h12_cfg);
wire_engine(s42_compusdt_tsmom_h12);

// AAVE-TSMOM-H8 minPF=2.16 score=110155
chimera::EdgeEngine::Config s42_aaveusdt_tsmom_h8_cfg{
    .symbol="aaveusdt", .tag="AAVE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_aaveusdt_tsmom_h8(s42_aaveusdt_tsmom_h8_cfg);
wire_engine(s42_aaveusdt_tsmom_h8);

// APT-TSMOM-H4 minPF=2.15 score=109912
chimera::EdgeEngine::Config s42_aptusdt_tsmom_h4_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_aptusdt_tsmom_h4(s42_aptusdt_tsmom_h4_cfg);
wire_engine(s42_aptusdt_tsmom_h4);

// CRV-TSMOM-H12 minPF=2.65 score=109896
chimera::EdgeEngine::Config s42_crvusdt_tsmom_h12_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_crvusdt_tsmom_h12(s42_crvusdt_tsmom_h12_cfg);
wire_engine(s42_crvusdt_tsmom_h12);

// JUP-TSMOM-H8 minPF=2.01 score=107736
chimera::EdgeEngine::Config s42_jupusdt_tsmom_h8_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jupusdt_tsmom_h8(s42_jupusdt_tsmom_h8_cfg);
wire_engine(s42_jupusdt_tsmom_h8);

// PYTH-TSMOM-H12 minPF=2.34 score=107097
chimera::EdgeEngine::Config s42_pythusdt_tsmom_h12_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_pythusdt_tsmom_h12(s42_pythusdt_tsmom_h12_cfg);
wire_engine(s42_pythusdt_tsmom_h12);

// EIGEN-TSMOM-H8 minPF=2.82 score=107038
chimera::EdgeEngine::Config s42_eigenusdt_tsmom_h8_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_tsmom_h8(s42_eigenusdt_tsmom_h8_cfg);
wire_engine(s42_eigenusdt_tsmom_h8);

// PYTH-TSMOM-H4 minPF=2.01 score=106672
chimera::EdgeEngine::Config s42_pythusdt_tsmom_h4_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_pythusdt_tsmom_h4(s42_pythusdt_tsmom_h4_cfg);
wire_engine(s42_pythusdt_tsmom_h4);

// EIGEN-TSMOM-H4 minPF=2.34 score=106114
chimera::EdgeEngine::Config s42_eigenusdt_tsmom_h4_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_tsmom_h4(s42_eigenusdt_tsmom_h4_cfg);
wire_engine(s42_eigenusdt_tsmom_h4);

// ETHFI-TSMOM-H12 minPF=2.16 score=104892
chimera::EdgeEngine::Config s42_ethfiusdt_tsmom_h12_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ethfiusdt_tsmom_h12(s42_ethfiusdt_tsmom_h12_cfg);
wire_engine(s42_ethfiusdt_tsmom_h12);

// IMX-TSMOM-H12 minPF=2.70 score=103267
chimera::EdgeEngine::Config s42_imxusdt_tsmom_h12_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_imxusdt_tsmom_h12(s42_imxusdt_tsmom_h12_cfg);
wire_engine(s42_imxusdt_tsmom_h12);

// MANA-TSMOM-H4 minPF=2.01 score=102167
chimera::EdgeEngine::Config s42_manausdt_tsmom_h4_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_manausdt_tsmom_h4(s42_manausdt_tsmom_h4_cfg);
wire_engine(s42_manausdt_tsmom_h4);

// EIGEN-TSMOM-H6 minPF=2.74 score=101128
chimera::EdgeEngine::Config s42_eigenusdt_tsmom_h6_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_tsmom_h6(s42_eigenusdt_tsmom_h6_cfg);
wire_engine(s42_eigenusdt_tsmom_h6);

// STX-TSMOM-H12 minPF=2.13 score=100620
chimera::EdgeEngine::Config s42_stxusdt_tsmom_h12_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_stxusdt_tsmom_h12(s42_stxusdt_tsmom_h12_cfg);
wire_engine(s42_stxusdt_tsmom_h12);

// XLM-TSMOM-H4 minPF=2.01 score=99926
chimera::EdgeEngine::Config s42_xlmusdt_tsmom_h4_cfg{
    .symbol="xlmusdt", .tag="XLM-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_xlmusdt_tsmom_h4(s42_xlmusdt_tsmom_h4_cfg);
wire_engine(s42_xlmusdt_tsmom_h4);

// FET-ICHI-H3 minPF=2.02 score=99561
chimera::EdgeEngine::Config s42_fetusdt_ichi_h3_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_fetusdt_ichi_h3(s42_fetusdt_ichi_h3_cfg);
wire_engine(s42_fetusdt_ichi_h3);

// MASK-TSMOM-H12 minPF=2.27 score=98631
chimera::EdgeEngine::Config s42_maskusdt_tsmom_h12_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_maskusdt_tsmom_h12(s42_maskusdt_tsmom_h12_cfg);
wire_engine(s42_maskusdt_tsmom_h12);

// ZRO-TSMOM-D1 minPF=2.38 score=98374
chimera::EdgeEngine::Config s42_zrousdt_tsmom_d1_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_tsmom_d1(s42_zrousdt_tsmom_d1_cfg);
wire_engine(s42_zrousdt_tsmom_d1);

// RUNE-TSMOM-H4 minPF=2.02 score=98210
chimera::EdgeEngine::Config s42_runeusdt_tsmom_h4_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_runeusdt_tsmom_h4(s42_runeusdt_tsmom_h4_cfg);
wire_engine(s42_runeusdt_tsmom_h4);

// ZRO-TSMOM-H6 minPF=2.02 score=98104
chimera::EdgeEngine::Config s42_zrousdt_tsmom_h6_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_tsmom_h6(s42_zrousdt_tsmom_h6_cfg);
wire_engine(s42_zrousdt_tsmom_h6);

// SAND-TSMOM-H6 minPF=2.12 score=97606
chimera::EdgeEngine::Config s42_sandusdt_tsmom_h6_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_sandusdt_tsmom_h6(s42_sandusdt_tsmom_h6_cfg);
wire_engine(s42_sandusdt_tsmom_h6);

// ATOM-TSMOM-H8 minPF=2.78 score=97496
chimera::EdgeEngine::Config s42_atomusdt_tsmom_h8_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_atomusdt_tsmom_h8(s42_atomusdt_tsmom_h8_cfg);
wire_engine(s42_atomusdt_tsmom_h8);

// ETHFI-TSMOM-D1 minPF=2.40 score=97345
chimera::EdgeEngine::Config s42_ethfiusdt_tsmom_d1_cfg{
    .symbol="ethfiusdt", .tag="ETHFI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ethfiusdt_tsmom_d1(s42_ethfiusdt_tsmom_d1_cfg);
wire_engine(s42_ethfiusdt_tsmom_d1);

// ENA-TSMOM-D1 minPF=3.05 score=96999
chimera::EdgeEngine::Config s42_enausdt_tsmom_d1_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_enausdt_tsmom_d1(s42_enausdt_tsmom_d1_cfg);
wire_engine(s42_enausdt_tsmom_d1);

// ICP-TSMOM-H12 minPF=2.15 score=96377
chimera::EdgeEngine::Config s42_icpusdt_tsmom_h12_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_icpusdt_tsmom_h12(s42_icpusdt_tsmom_h12_cfg);
wire_engine(s42_icpusdt_tsmom_h12);

// ARKM-TSMOM-H8 minPF=2.15 score=95482
chimera::EdgeEngine::Config s42_arkmusdt_tsmom_h8_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_arkmusdt_tsmom_h8(s42_arkmusdt_tsmom_h8_cfg);
wire_engine(s42_arkmusdt_tsmom_h8);

// JTO-TSMOM-D1 minPF=2.25 score=95203
chimera::EdgeEngine::Config s42_jtousdt_tsmom_d1_cfg{
    .symbol="jtousdt", .tag="JTO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_tsmom_d1(s42_jtousdt_tsmom_d1_cfg);
wire_engine(s42_jtousdt_tsmom_d1);

// RENDER-TSMOM-H3 minPF=2.24 score=94601
chimera::EdgeEngine::Config s42_renderusdt_tsmom_h3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_tsmom_h3(s42_renderusdt_tsmom_h3_cfg);
wire_engine(s42_renderusdt_tsmom_h3);

// DOT-TSMOM-H3 minPF=2.04 score=94304
chimera::EdgeEngine::Config s42_dotusdt_tsmom_h3_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_dotusdt_tsmom_h3(s42_dotusdt_tsmom_h3_cfg);
wire_engine(s42_dotusdt_tsmom_h3);

// ADA-TSMOM-H6 minPF=2.17 score=93543
chimera::EdgeEngine::Config s42_adausdt_tsmom_h6_cfg{
    .symbol="adausdt", .tag="ADA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_adausdt_tsmom_h6(s42_adausdt_tsmom_h6_cfg);
wire_engine(s42_adausdt_tsmom_h6);

// GMT-TSMOM-H12 minPF=2.13 score=92871
chimera::EdgeEngine::Config s42_gmtusdt_tsmom_h12_cfg{
    .symbol="gmtusdt", .tag="GMT-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_gmtusdt_tsmom_h12(s42_gmtusdt_tsmom_h12_cfg);
wire_engine(s42_gmtusdt_tsmom_h12);

// EIGEN-TSMOM-H12 minPF=3.47 score=92797
chimera::EdgeEngine::Config s42_eigenusdt_tsmom_h12_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_tsmom_h12(s42_eigenusdt_tsmom_h12_cfg);
wire_engine(s42_eigenusdt_tsmom_h12);

// SAND-TSMOM-H8 minPF=2.10 score=92454
chimera::EdgeEngine::Config s42_sandusdt_tsmom_h8_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_sandusdt_tsmom_h8(s42_sandusdt_tsmom_h8_cfg);
wire_engine(s42_sandusdt_tsmom_h8);

// TURBO-TSMOM-H12 minPF=2.38 score=92028
chimera::EdgeEngine::Config s42_turbousdt_tsmom_h12_cfg{
    .symbol="turbousdt", .tag="TURBO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_turbousdt_tsmom_h12(s42_turbousdt_tsmom_h12_cfg);
wire_engine(s42_turbousdt_tsmom_h12);

// JUP-TSMOM-D1 minPF=2.80 score=91908
chimera::EdgeEngine::Config s42_jupusdt_tsmom_d1_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jupusdt_tsmom_d1(s42_jupusdt_tsmom_d1_cfg);
wire_engine(s42_jupusdt_tsmom_d1);

// UNI-TSMOM-D1 minPF=2.21 score=91056
chimera::EdgeEngine::Config s42_uniusdt_tsmom_d1_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_uniusdt_tsmom_d1(s42_uniusdt_tsmom_d1_cfg);
wire_engine(s42_uniusdt_tsmom_d1);

// ARB-TSMOM-H12 minPF=2.29 score=90533
chimera::EdgeEngine::Config s42_arbusdt_tsmom_h12_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_arbusdt_tsmom_h12(s42_arbusdt_tsmom_h12_cfg);
wire_engine(s42_arbusdt_tsmom_h12);

// SOL-TSMOM-H8 minPF=2.75 score=90469
chimera::EdgeEngine::Config s42_solusdt_tsmom_h8_cfg{
    .symbol="solusdt", .tag="SOL-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_solusdt_tsmom_h8(s42_solusdt_tsmom_h8_cfg);
wire_engine(s42_solusdt_tsmom_h8);

// FET-ICHI-H4 minPF=2.55 score=89113
chimera::EdgeEngine::Config s42_fetusdt_ichi_h4_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=45, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_fetusdt_ichi_h4(s42_fetusdt_ichi_h4_cfg);
wire_engine(s42_fetusdt_ichi_h4);

// JTO-ICHI-H6 minPF=2.01 score=88873
chimera::EdgeEngine::Config s42_jtousdt_ichi_h6_cfg{
    .symbol="jtousdt", .tag="JTO-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_ichi_h6(s42_jtousdt_ichi_h6_cfg);
wire_engine(s42_jtousdt_ichi_h6);

// JTO-BOPB-H4 minPF=3.16 score=88170
chimera::EdgeEngine::Config s42_jtousdt_bopb_h4_cfg{
    .symbol="jtousdt", .tag="JTO-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_bopb_h4(s42_jtousdt_bopb_h4_cfg);
wire_engine(s42_jtousdt_bopb_h4);

// RENDER-TSMOM-H12 minPF=4.39 score=88127
chimera::EdgeEngine::Config s42_renderusdt_tsmom_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_tsmom_h12(s42_renderusdt_tsmom_h12_cfg);
wire_engine(s42_renderusdt_tsmom_h12);

// W-TSMOM-H12 minPF=2.26 score=88061
chimera::EdgeEngine::Config s42_wusdt_tsmom_h12_cfg{
    .symbol="wusdt", .tag="W-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_wusdt_tsmom_h12(s42_wusdt_tsmom_h12_cfg);
wire_engine(s42_wusdt_tsmom_h12);

// DOT-TSMOM-H6 minPF=2.17 score=87438
chimera::EdgeEngine::Config s42_dotusdt_tsmom_h6_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_dotusdt_tsmom_h6(s42_dotusdt_tsmom_h6_cfg);
wire_engine(s42_dotusdt_tsmom_h6);

// TIA-ICHI-H3 minPF=2.09 score=87112
chimera::EdgeEngine::Config s42_tiausdt_ichi_h3_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_ichi_h3(s42_tiausdt_ichi_h3_cfg);
wire_engine(s42_tiausdt_ichi_h3);

// XLM-TSMOM-H8 minPF=2.46 score=86993
chimera::EdgeEngine::Config s42_xlmusdt_tsmom_h8_cfg{
    .symbol="xlmusdt", .tag="XLM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_xlmusdt_tsmom_h8(s42_xlmusdt_tsmom_h8_cfg);
wire_engine(s42_xlmusdt_tsmom_h8);

// INJ-ICHI-H4 minPF=2.10 score=86682
chimera::EdgeEngine::Config s42_injusdt_ichi_h4_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_injusdt_ichi_h4(s42_injusdt_ichi_h4_cfg);
wire_engine(s42_injusdt_ichi_h4);

// WIF-TSMOM-D1 minPF=2.02 score=86581
chimera::EdgeEngine::Config s42_wifusdt_tsmom_d1_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_wifusdt_tsmom_d1(s42_wifusdt_tsmom_d1_cfg);
wire_engine(s42_wifusdt_tsmom_d1);

// RENDER-TSMOM-H8 minPF=2.38 score=86114
chimera::EdgeEngine::Config s42_renderusdt_tsmom_h8_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_tsmom_h8(s42_renderusdt_tsmom_h8_cfg);
wire_engine(s42_renderusdt_tsmom_h8);

// IMX-TSMOM-D1 minPF=2.25 score=85850
chimera::EdgeEngine::Config s42_imxusdt_tsmom_d1_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_imxusdt_tsmom_d1(s42_imxusdt_tsmom_d1_cfg);
wire_engine(s42_imxusdt_tsmom_d1);

// ARKM-TSMOM-D1 minPF=2.56 score=85354
chimera::EdgeEngine::Config s42_arkmusdt_tsmom_d1_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_arkmusdt_tsmom_d1(s42_arkmusdt_tsmom_d1_cfg);
wire_engine(s42_arkmusdt_tsmom_d1);

// ATOM-TSMOM-H6 minPF=2.00 score=83702
chimera::EdgeEngine::Config s42_atomusdt_tsmom_h6_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_atomusdt_tsmom_h6(s42_atomusdt_tsmom_h6_cfg);
wire_engine(s42_atomusdt_tsmom_h6);

// RUNE-TSMOM-H8 minPF=2.16 score=82517
chimera::EdgeEngine::Config s42_runeusdt_tsmom_h8_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_runeusdt_tsmom_h8(s42_runeusdt_tsmom_h8_cfg);
wire_engine(s42_runeusdt_tsmom_h8);

// MANA-TSMOM-H12 minPF=2.03 score=82467
chimera::EdgeEngine::Config s42_manausdt_tsmom_h12_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_manausdt_tsmom_h12(s42_manausdt_tsmom_h12_cfg);
wire_engine(s42_manausdt_tsmom_h12);

// ATOM-TSMOM-H12 minPF=2.84 score=79333
chimera::EdgeEngine::Config s42_atomusdt_tsmom_h12_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_atomusdt_tsmom_h12(s42_atomusdt_tsmom_h12_cfg);
wire_engine(s42_atomusdt_tsmom_h12);

// HBAR-TSMOM-H4 minPF=2.01 score=79081
chimera::EdgeEngine::Config s42_hbarusdt_tsmom_h4_cfg{
    .symbol="hbarusdt", .tag="HBAR-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_hbarusdt_tsmom_h4(s42_hbarusdt_tsmom_h4_cfg);
wire_engine(s42_hbarusdt_tsmom_h4);

// RUNE-TSMOM-H12 minPF=2.67 score=78965
chimera::EdgeEngine::Config s42_runeusdt_tsmom_h12_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_runeusdt_tsmom_h12(s42_runeusdt_tsmom_h12_cfg);
wire_engine(s42_runeusdt_tsmom_h12);

// ARB-TSMOM-D1 minPF=3.58 score=78715
chimera::EdgeEngine::Config s42_arbusdt_tsmom_d1_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_arbusdt_tsmom_d1(s42_arbusdt_tsmom_d1_cfg);
wire_engine(s42_arbusdt_tsmom_d1);

// DOT-TSMOM-H12 minPF=2.13 score=78263
chimera::EdgeEngine::Config s42_dotusdt_tsmom_h12_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_dotusdt_tsmom_h12(s42_dotusdt_tsmom_h12_cfg);
wire_engine(s42_dotusdt_tsmom_h12);

// ICP-ICHI-H4 minPF=2.03 score=78137
chimera::EdgeEngine::Config s42_icpusdt_ichi_h4_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=12, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_icpusdt_ichi_h4(s42_icpusdt_ichi_h4_cfg);
wire_engine(s42_icpusdt_ichi_h4);

// MANA-TSMOM-H8 minPF=2.00 score=78086
chimera::EdgeEngine::Config s42_manausdt_tsmom_h8_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_manausdt_tsmom_h8(s42_manausdt_tsmom_h8_cfg);
wire_engine(s42_manausdt_tsmom_h8);

// FIL-TSMOM-D1 minPF=2.36 score=77606
chimera::EdgeEngine::Config s42_filusdt_tsmom_d1_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_filusdt_tsmom_d1(s42_filusdt_tsmom_d1_cfg);
wire_engine(s42_filusdt_tsmom_d1);

// INJ-ICHI-H6 minPF=2.01 score=76791
chimera::EdgeEngine::Config s42_injusdt_ichi_h6_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_injusdt_ichi_h6(s42_injusdt_ichi_h6_cfg);
wire_engine(s42_injusdt_ichi_h6);

// XLM-TSMOM-H6 minPF=2.07 score=76682
chimera::EdgeEngine::Config s42_xlmusdt_tsmom_h6_cfg{
    .symbol="xlmusdt", .tag="XLM-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_xlmusdt_tsmom_h6(s42_xlmusdt_tsmom_h6_cfg);
wire_engine(s42_xlmusdt_tsmom_h6);

// MASK-TSMOM-D1 minPF=2.34 score=76606
chimera::EdgeEngine::Config s42_maskusdt_tsmom_d1_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_maskusdt_tsmom_d1(s42_maskusdt_tsmom_d1_cfg);
wire_engine(s42_maskusdt_tsmom_d1);

// SUI-TSMOM-D1 minPF=2.09 score=74227
chimera::EdgeEngine::Config s42_suiusdt_tsmom_d1_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_suiusdt_tsmom_d1(s42_suiusdt_tsmom_d1_cfg);
wire_engine(s42_suiusdt_tsmom_d1);

// TIA-ICHI-H4 minPF=2.09 score=73191
chimera::EdgeEngine::Config s42_tiausdt_ichi_h4_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_ichi_h4(s42_tiausdt_ichi_h4_cfg);
wire_engine(s42_tiausdt_ichi_h4);

// ZRO-TSMOM-H12 minPF=2.74 score=72932
chimera::EdgeEngine::Config s42_zrousdt_tsmom_h12_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_tsmom_h12(s42_zrousdt_tsmom_h12_cfg);
wire_engine(s42_zrousdt_tsmom_h12);

// ETH-TSMOM-H12 minPF=2.50 score=72332
chimera::EdgeEngine::Config s42_ethusdt_tsmom_h12_cfg{
    .symbol="ethusdt", .tag="ETH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=17, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ethusdt_tsmom_h12(s42_ethusdt_tsmom_h12_cfg);
wire_engine(s42_ethusdt_tsmom_h12);

// MASK-ICHI-H3 minPF=2.11 score=72103
chimera::EdgeEngine::Config s42_maskusdt_ichi_h3_cfg{
    .symbol="maskusdt", .tag="MASK-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_maskusdt_ichi_h3(s42_maskusdt_ichi_h3_cfg);
wire_engine(s42_maskusdt_ichi_h3);

// SUI-ICHI-H8 minPF=2.65 score=72059
chimera::EdgeEngine::Config s42_suiusdt_ichi_h8_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=30, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_suiusdt_ichi_h8(s42_suiusdt_ichi_h8_cfg);
wire_engine(s42_suiusdt_ichi_h8);

// TIA-ICHI-H6 minPF=2.44 score=71773
chimera::EdgeEngine::Config s42_tiausdt_ichi_h6_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_ichi_h6(s42_tiausdt_ichi_h6_cfg);
wire_engine(s42_tiausdt_ichi_h6);

// OP-TSMOM-D1 minPF=2.23 score=70968
chimera::EdgeEngine::Config s42_opusdt_tsmom_d1_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_opusdt_tsmom_d1(s42_opusdt_tsmom_d1_cfg);
wire_engine(s42_opusdt_tsmom_d1);

// FLOKI-TSMOM-D1 minPF=2.16 score=70957
chimera::EdgeEngine::Config s42_flokiusdt_tsmom_d1_cfg{
    .symbol="flokiusdt", .tag="FLOKI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_tsmom_d1(s42_flokiusdt_tsmom_d1_cfg);
wire_engine(s42_flokiusdt_tsmom_d1);

// APT-TSMOM-D1 minPF=2.26 score=69688
chimera::EdgeEngine::Config s42_aptusdt_tsmom_d1_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_aptusdt_tsmom_d1(s42_aptusdt_tsmom_d1_cfg);
wire_engine(s42_aptusdt_tsmom_d1);

// XLM-ICHI-H4 minPF=2.54 score=67644
chimera::EdgeEngine::Config s42_xlmusdt_ichi_h4_cfg{
    .symbol="xlmusdt", .tag="XLM-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_xlmusdt_ichi_h4(s42_xlmusdt_ichi_h4_cfg);
wire_engine(s42_xlmusdt_ichi_h4);

// LTC-TSMOM-H12 minPF=2.47 score=67368
chimera::EdgeEngine::Config s42_ltcusdt_tsmom_h12_cfg{
    .symbol="ltcusdt", .tag="LTC-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ltcusdt_tsmom_h12(s42_ltcusdt_tsmom_h12_cfg);
wire_engine(s42_ltcusdt_tsmom_h12);

// MANA-TSMOM-D1 minPF=2.54 score=67137
chimera::EdgeEngine::Config s42_manausdt_tsmom_d1_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_manausdt_tsmom_d1(s42_manausdt_tsmom_d1_cfg);
wire_engine(s42_manausdt_tsmom_d1);

// WIF-ICHI-H6 minPF=2.04 score=66512
chimera::EdgeEngine::Config s42_wifusdt_ichi_h6_cfg{
    .symbol="wifusdt", .tag="WIF-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_wifusdt_ichi_h6(s42_wifusdt_ichi_h6_cfg);
wire_engine(s42_wifusdt_ichi_h6);

// OP-TSMOM-H12 minPF=2.00 score=66097
chimera::EdgeEngine::Config s42_opusdt_tsmom_h12_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_opusdt_tsmom_h12(s42_opusdt_tsmom_h12_cfg);
wire_engine(s42_opusdt_tsmom_h12);

// RENDER-ICHI-H2 minPF=2.03 score=65876
chimera::EdgeEngine::Config s42_renderusdt_ichi_h2_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=12, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_ichi_h2(s42_renderusdt_ichi_h2_cfg);
wire_engine(s42_renderusdt_ichi_h2);

// FLOKI-ICHI-H8 minPF=2.07 score=64506
chimera::EdgeEngine::Config s42_flokiusdt_ichi_h8_cfg{
    .symbol="flokiusdt", .tag="FLOKI-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=18, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_ichi_h8(s42_flokiusdt_ichi_h8_cfg);
wire_engine(s42_flokiusdt_ichi_h8);

// EIGEN-TSMOM-D1 minPF=3.62 score=62715
chimera::EdgeEngine::Config s42_eigenusdt_tsmom_d1_cfg{
    .symbol="eigenusdt", .tag="EIGEN-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_tsmom_d1(s42_eigenusdt_tsmom_d1_cfg);
wire_engine(s42_eigenusdt_tsmom_d1);

// STX-TSMOM-D1 minPF=2.06 score=61571
chimera::EdgeEngine::Config s42_stxusdt_tsmom_d1_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_stxusdt_tsmom_d1(s42_stxusdt_tsmom_d1_cfg);
wire_engine(s42_stxusdt_tsmom_d1);

// ZRO-TSMOM-D2 minPF=6.35 score=61275
chimera::EdgeEngine::Config s42_zrousdt_tsmom_d2_cfg{
    .symbol="zrousdt", .tag="ZRO-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=45, .hold_bars=5, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_tsmom_d2(s42_zrousdt_tsmom_d2_cfg);
wire_engine(s42_zrousdt_tsmom_d2);

// CRV-TSMOM-D1 minPF=2.35 score=61102
chimera::EdgeEngine::Config s42_crvusdt_tsmom_d1_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_crvusdt_tsmom_d1(s42_crvusdt_tsmom_d1_cfg);
wire_engine(s42_crvusdt_tsmom_d1);

// ONDO-TSMOM-H4 minPF=2.17 score=61069
chimera::EdgeEngine::Config s42_ondousdt_tsmom_h4_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_tsmom_h4(s42_ondousdt_tsmom_h4_cfg);
wire_engine(s42_ondousdt_tsmom_h4);

// BCH-TSMOM-D1 minPF=2.74 score=60400
chimera::EdgeEngine::Config s42_bchusdt_tsmom_d1_cfg{
    .symbol="bchusdt", .tag="BCH-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bchusdt_tsmom_d1(s42_bchusdt_tsmom_d1_cfg);
wire_engine(s42_bchusdt_tsmom_d1);

// RENDER-ICHI-H3 minPF=2.14 score=60274
chimera::EdgeEngine::Config s42_renderusdt_ichi_h3_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_ichi_h3(s42_renderusdt_ichi_h3_cfg);
wire_engine(s42_renderusdt_ichi_h3);

// ZRO-ICHI-H3 minPF=2.03 score=59662
chimera::EdgeEngine::Config s42_zrousdt_ichi_h3_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_ichi_h3(s42_zrousdt_ichi_h3_cfg);
wire_engine(s42_zrousdt_ichi_h3);

// AVAX-TSMOM-D1 minPF=2.42 score=59618
chimera::EdgeEngine::Config s42_avaxusdt_tsmom_d1_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_avaxusdt_tsmom_d1(s42_avaxusdt_tsmom_d1_cfg);
wire_engine(s42_avaxusdt_tsmom_d1);

// JTO-ICHI-H8 minPF=2.06 score=57854
chimera::EdgeEngine::Config s42_jtousdt_ichi_h8_cfg{
    .symbol="jtousdt", .tag="JTO-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=12, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_jtousdt_ichi_h8(s42_jtousdt_ichi_h8_cfg);
wire_engine(s42_jtousdt_ichi_h8);

// ZRO-ICHI-H4 minPF=2.35 score=56649
chimera::EdgeEngine::Config s42_zrousdt_ichi_h4_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_ichi_h4(s42_zrousdt_ichi_h4_cfg);
wire_engine(s42_zrousdt_ichi_h4);

// SHIB-TSMOM-H8 minPF=2.01 score=56146
chimera::EdgeEngine::Config s42_shibusdt_tsmom_h8_cfg{
    .symbol="shibusdt", .tag="SHIB-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_shibusdt_tsmom_h8(s42_shibusdt_tsmom_h8_cfg);
wire_engine(s42_shibusdt_tsmom_h8);

// SHIB-TSMOM-H12 minPF=2.14 score=56061
chimera::EdgeEngine::Config s42_shibusdt_tsmom_h12_cfg{
    .symbol="shibusdt", .tag="SHIB-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_shibusdt_tsmom_h12(s42_shibusdt_tsmom_h12_cfg);
wire_engine(s42_shibusdt_tsmom_h12);

// RUNE-TSMOM-D1 minPF=2.12 score=56056
chimera::EdgeEngine::Config s42_runeusdt_tsmom_d1_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_runeusdt_tsmom_d1(s42_runeusdt_tsmom_d1_cfg);
wire_engine(s42_runeusdt_tsmom_d1);

// ATOM-ICHI-H4 minPF=2.25 score=55736
chimera::EdgeEngine::Config s42_atomusdt_ichi_h4_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_atomusdt_ichi_h4(s42_atomusdt_ichi_h4_cfg);
wire_engine(s42_atomusdt_ichi_h4);

// XLM-TSMOM-D1 minPF=2.05 score=54178
chimera::EdgeEngine::Config s42_xlmusdt_tsmom_d1_cfg{
    .symbol="xlmusdt", .tag="XLM-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_xlmusdt_tsmom_d1(s42_xlmusdt_tsmom_d1_cfg);
wire_engine(s42_xlmusdt_tsmom_d1);

// ATOM-TSMOM-D2 minPF=4.28 score=53014
chimera::EdgeEngine::Config s42_atomusdt_tsmom_d2_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_atomusdt_tsmom_d2(s42_atomusdt_tsmom_d2_cfg);
wire_engine(s42_atomusdt_tsmom_d2);

// TON-TSMOM-H8 minPF=2.14 score=52938
chimera::EdgeEngine::Config s42_tonusdt_tsmom_h8_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tonusdt_tsmom_h8(s42_tonusdt_tsmom_h8_cfg);
wire_engine(s42_tonusdt_tsmom_h8);

// BCH-TSMOM-H12 minPF=2.00 score=52867
chimera::EdgeEngine::Config s42_bchusdt_tsmom_h12_cfg{
    .symbol="bchusdt", .tag="BCH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_bchusdt_tsmom_h12(s42_bchusdt_tsmom_h12_cfg);
wire_engine(s42_bchusdt_tsmom_h12);

// FLOKI-ICHI-H12 minPF=2.08 score=51804
chimera::EdgeEngine::Config s42_flokiusdt_ichi_h12_cfg{
    .symbol="flokiusdt", .tag="FLOKI-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_flokiusdt_ichi_h12(s42_flokiusdt_ichi_h12_cfg);
wire_engine(s42_flokiusdt_ichi_h12);

// DOGE-ICHI-H8 minPF=2.26 score=51792
chimera::EdgeEngine::Config s42_dogeusdt_ichi_h8_cfg{
    .symbol="dogeusdt", .tag="DOGE-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_dogeusdt_ichi_h8(s42_dogeusdt_ichi_h8_cfg);
wire_engine(s42_dogeusdt_ichi_h8);

// OP-TSMOM-D2 minPF=2.29 score=50511
chimera::EdgeEngine::Config s42_opusdt_tsmom_d2_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_opusdt_tsmom_d2(s42_opusdt_tsmom_d2_cfg);
wire_engine(s42_opusdt_tsmom_d2);

// RUNE-ICHI-H8 minPF=2.77 score=50389
chimera::EdgeEngine::Config s42_runeusdt_ichi_h8_cfg{
    .symbol="runeusdt", .tag="RUNE-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=18, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_runeusdt_ichi_h8(s42_runeusdt_ichi_h8_cfg);
wire_engine(s42_runeusdt_ichi_h8);

// OP-ICHI-H8 minPF=2.06 score=50160
chimera::EdgeEngine::Config s42_opusdt_ichi_h8_cfg{
    .symbol="opusdt", .tag="OP-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_opusdt_ichi_h8(s42_opusdt_ichi_h8_cfg);
wire_engine(s42_opusdt_ichi_h8);

// ONDO-TSMOM-H6 minPF=2.29 score=49851
chimera::EdgeEngine::Config s42_ondousdt_tsmom_h6_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_tsmom_h6(s42_ondousdt_tsmom_h6_cfg);
wire_engine(s42_ondousdt_tsmom_h6);

// SOL-ICHI-H6 minPF=2.10 score=49336
chimera::EdgeEngine::Config s42_solusdt_ichi_h6_cfg{
    .symbol="solusdt", .tag="SOL-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_solusdt_ichi_h6(s42_solusdt_ichi_h6_cfg);
wire_engine(s42_solusdt_ichi_h6);

// XLM-ICHI-H8 minPF=2.05 score=49099
chimera::EdgeEngine::Config s42_xlmusdt_ichi_h8_cfg{
    .symbol="xlmusdt", .tag="XLM-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_xlmusdt_ichi_h8(s42_xlmusdt_ichi_h8_cfg);
wire_engine(s42_xlmusdt_ichi_h8);

// ONDO-ICHI-H3 minPF=2.32 score=49070
chimera::EdgeEngine::Config s42_ondousdt_ichi_h3_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=12, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_ichi_h3(s42_ondousdt_ichi_h3_cfg);
wire_engine(s42_ondousdt_ichi_h3);

// RENDER-TSMOM-D2 minPF=3.19 score=48785
chimera::EdgeEngine::Config s42_renderusdt_tsmom_d2_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_tsmom_d2(s42_renderusdt_tsmom_d2_cfg);
wire_engine(s42_renderusdt_tsmom_d2);

// TIA-TSMOM-D1 minPF=3.33 score=47390
chimera::EdgeEngine::Config s42_tiausdt_tsmom_d1_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_tsmom_d1(s42_tiausdt_tsmom_d1_cfg);
wire_engine(s42_tiausdt_tsmom_d1);

// ETC-TSMOM-D1 minPF=2.26 score=47033
chimera::EdgeEngine::Config s42_etcusdt_tsmom_d1_cfg{
    .symbol="etcusdt", .tag="ETC-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_etcusdt_tsmom_d1(s42_etcusdt_tsmom_d1_cfg);
wire_engine(s42_etcusdt_tsmom_d1);

// RUNE-TSMOM-D2 minPF=2.08 score=46973
chimera::EdgeEngine::Config s42_runeusdt_tsmom_d2_cfg{
    .symbol="runeusdt", .tag="RUNE-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_runeusdt_tsmom_d2(s42_runeusdt_tsmom_d2_cfg);
wire_engine(s42_runeusdt_tsmom_d2);

// ATOM-TSMOM-D1 minPF=2.22 score=46930
chimera::EdgeEngine::Config s42_atomusdt_tsmom_d1_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_atomusdt_tsmom_d1(s42_atomusdt_tsmom_d1_cfg);
wire_engine(s42_atomusdt_tsmom_d1);

// TRX-TSMOM-H12 minPF=2.08 score=46111
chimera::EdgeEngine::Config s42_trxusdt_tsmom_h12_cfg{
    .symbol="trxusdt", .tag="TRX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_trxusdt_tsmom_h12(s42_trxusdt_tsmom_h12_cfg);
wire_engine(s42_trxusdt_tsmom_h12);

// ONDO-TSMOM-D1 minPF=2.50 score=45732
chimera::EdgeEngine::Config s42_ondousdt_tsmom_d1_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_tsmom_d1(s42_ondousdt_tsmom_d1_cfg);
wire_engine(s42_ondousdt_tsmom_d1);

// TON-TSMOM-H6 minPF=2.14 score=44810
chimera::EdgeEngine::Config s42_tonusdt_tsmom_h6_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tonusdt_tsmom_h6(s42_tonusdt_tsmom_h6_cfg);
wire_engine(s42_tonusdt_tsmom_h6);

// ZRO-ICHI-D2 minPF=3.53 score=44598
chimera::EdgeEngine::Config s42_zrousdt_ichi_d2_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-D2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=172800, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_ichi_d2(s42_zrousdt_ichi_d2_cfg);
wire_engine(s42_zrousdt_ichi_d2);

// ZRO-ICHI-H8 minPF=2.97 score=44568
chimera::EdgeEngine::Config s42_zrousdt_ichi_h8_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_ichi_h8(s42_zrousdt_ichi_h8_cfg);
wire_engine(s42_zrousdt_ichi_h8);

// VET-TSMOM-D1 minPF=2.10 score=44104
chimera::EdgeEngine::Config s42_vetusdt_tsmom_d1_cfg{
    .symbol="vetusdt", .tag="VET-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_vetusdt_tsmom_d1(s42_vetusdt_tsmom_d1_cfg);
wire_engine(s42_vetusdt_tsmom_d1);

// LTC-TSMOM-D1 minPF=2.25 score=42190
chimera::EdgeEngine::Config s42_ltcusdt_tsmom_d1_cfg{
    .symbol="ltcusdt", .tag="LTC-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ltcusdt_tsmom_d1(s42_ltcusdt_tsmom_d1_cfg);
wire_engine(s42_ltcusdt_tsmom_d1);

// TRX-TSMOM-D3 minPF=2.87 score=38996
chimera::EdgeEngine::Config s42_trxusdt_tsmom_d3_cfg{
    .symbol="trxusdt", .tag="TRX-TSMOM-D3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=259200, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_trxusdt_tsmom_d3(s42_trxusdt_tsmom_d3_cfg);
wire_engine(s42_trxusdt_tsmom_d3);

// LTC-ICHI-H12 minPF=3.11 score=38706
chimera::EdgeEngine::Config s42_ltcusdt_ichi_h12_cfg{
    .symbol="ltcusdt", .tag="LTC-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ltcusdt_ichi_h12(s42_ltcusdt_ichi_h12_cfg);
wire_engine(s42_ltcusdt_ichi_h12);

// ZRO-WILLR-H6 minPF=2.15 score=38284
chimera::EdgeEngine::Config s42_zrousdt_willr_h6_cfg{
    .symbol="zrousdt", .tag="ZRO-WILLR-H6", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_willr_h6(s42_zrousdt_willr_h6_cfg);
wire_engine(s42_zrousdt_willr_h6);

// EIGEN-ICHI-H12 minPF=3.40 score=38147
chimera::EdgeEngine::Config s42_eigenusdt_ichi_h12_cfg{
    .symbol="eigenusdt", .tag="EIGEN-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_ichi_h12(s42_eigenusdt_ichi_h12_cfg);
wire_engine(s42_eigenusdt_ichi_h12);

// EIGEN-DCH-H8 minPF=3.19 score=38109
chimera::EdgeEngine::Config s42_eigenusdt_dch_h8_cfg{
    .symbol="eigenusdt", .tag="EIGEN-DCH-H8", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=28800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_dch_h8(s42_eigenusdt_dch_h8_cfg);
wire_engine(s42_eigenusdt_dch_h8);

// EIGEN-SR-H12 minPF=3.42 score=36341
chimera::EdgeEngine::Config s42_eigenusdt_sr_h12_cfg{
    .symbol="eigenusdt", .tag="EIGEN-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_sr_h12(s42_eigenusdt_sr_h12_cfg);
wire_engine(s42_eigenusdt_sr_h12);

// RENDER-ICHI-H12 minPF=2.00 score=36325
chimera::EdgeEngine::Config s42_renderusdt_ichi_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=12, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_ichi_h12(s42_renderusdt_ichi_h12_cfg);
wire_engine(s42_renderusdt_ichi_h12);

// RUNE-ICHI-H6 minPF=2.06 score=35350
chimera::EdgeEngine::Config s42_runeusdt_ichi_h6_cfg{
    .symbol="runeusdt", .tag="RUNE-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_runeusdt_ichi_h6(s42_runeusdt_ichi_h6_cfg);
wire_engine(s42_runeusdt_ichi_h6);

// SUI-BOPB-H4 minPF=2.15 score=33116
chimera::EdgeEngine::Config s42_suiusdt_bopb_h4_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_suiusdt_bopb_h4(s42_suiusdt_bopb_h4_cfg);
wire_engine(s42_suiusdt_bopb_h4);

// TON-TSMOM-D1 minPF=2.43 score=32926
chimera::EdgeEngine::Config s42_tonusdt_tsmom_d1_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tonusdt_tsmom_d1(s42_tonusdt_tsmom_d1_cfg);
wire_engine(s42_tonusdt_tsmom_d1);

// EIGEN-ICHI-H6 minPF=2.02 score=32575
chimera::EdgeEngine::Config s42_eigenusdt_ichi_h6_cfg{
    .symbol="eigenusdt", .tag="EIGEN-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_ichi_h6(s42_eigenusdt_ichi_h6_cfg);
wire_engine(s42_eigenusdt_ichi_h6);

// ONDO-ICHI-H4 minPF=2.30 score=32013
chimera::EdgeEngine::Config s42_ondousdt_ichi_h4_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_ichi_h4(s42_ondousdt_ichi_h4_cfg);
wire_engine(s42_ondousdt_ichi_h4);

// ZRO-ICHI-H12 minPF=2.09 score=30752
chimera::EdgeEngine::Config s42_zrousdt_ichi_h12_cfg{
    .symbol="zrousdt", .tag="ZRO-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_ichi_h12(s42_zrousdt_ichi_h12_cfg);
wire_engine(s42_zrousdt_ichi_h12);

// TIA-BOPB-H6 minPF=2.02 score=30637
chimera::EdgeEngine::Config s42_tiausdt_bopb_h6_cfg{
    .symbol="tiausdt", .tag="TIA-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_bopb_h6(s42_tiausdt_bopb_h6_cfg);
wire_engine(s42_tiausdt_bopb_h6);

// EIGEN-ICHI-H8 minPF=2.29 score=30557
chimera::EdgeEngine::Config s42_eigenusdt_ichi_h8_cfg{
    .symbol="eigenusdt", .tag="EIGEN-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_eigenusdt_ichi_h8(s42_eigenusdt_ichi_h8_cfg);
wire_engine(s42_eigenusdt_ichi_h8);

// UNI-BOLL-H2 minPF=2.31 score=28756
chimera::EdgeEngine::Config s42_uniusdt_boll_h2_cfg{
    .symbol="uniusdt", .tag="UNI-BOLL-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=18, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_uniusdt_boll_h2(s42_uniusdt_boll_h2_cfg);
wire_engine(s42_uniusdt_boll_h2);

// RENDER-WILLR-H12 minPF=2.83 score=26343
chimera::EdgeEngine::Config s42_renderusdt_willr_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-WILLR-H12", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=43200, .lookback=30, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_willr_h12(s42_renderusdt_willr_h12_cfg);
wire_engine(s42_renderusdt_willr_h12);

// FET-BOPB-H3 minPF=2.02 score=26108
chimera::EdgeEngine::Config s42_fetusdt_bopb_h3_cfg{
    .symbol="fetusdt", .tag="FET-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_fetusdt_bopb_h3(s42_fetusdt_bopb_h3_cfg);
wire_engine(s42_fetusdt_bopb_h3);

// ONDO-DT-H12 minPF=2.11 score=22841
chimera::EdgeEngine::Config s42_ondousdt_dt_h12_cfg{
    .symbol="ondousdt", .tag="ONDO-DT-H12", .kind=chimera::StrategyKind::DUAL_THRUST,
    .tf_secs=43200, .lookback=45, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_dt_h12(s42_ondousdt_dt_h12_cfg);
wire_engine(s42_ondousdt_dt_h12);

// SUI-BOPB-H6 minPF=2.03 score=21961
chimera::EdgeEngine::Config s42_suiusdt_bopb_h6_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_suiusdt_bopb_h6(s42_suiusdt_bopb_h6_cfg);
wire_engine(s42_suiusdt_bopb_h6);

// ONDO-ICHI-H12 minPF=2.37 score=21195
chimera::EdgeEngine::Config s42_ondousdt_ichi_h12_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=30, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_ondousdt_ichi_h12(s42_ondousdt_ichi_h12_cfg);
wire_engine(s42_ondousdt_ichi_h12);

// RENDER-BOPB-H8 minPF=2.42 score=21154
chimera::EdgeEngine::Config s42_renderusdt_bopb_h8_cfg{
    .symbol="renderusdt", .tag="RENDER-BOPB-H8", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=28800, .lookback=6, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_renderusdt_bopb_h8(s42_renderusdt_bopb_h8_cfg);
wire_engine(s42_renderusdt_bopb_h8);

// TURBO-KELT-H4 minPF=4.15 score=17337
chimera::EdgeEngine::Config s42_turbousdt_kelt_h4_cfg{
    .symbol="turbousdt", .tag="TURBO-KELT-H4", .kind=chimera::StrategyKind::KELTNER_REVERT,
    .tf_secs=14400, .lookback=30, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_turbousdt_kelt_h4(s42_turbousdt_kelt_h4_cfg);
wire_engine(s42_turbousdt_kelt_h4);

// TIA-BOPB-H3 minPF=2.53 score=16794
chimera::EdgeEngine::Config s42_tiausdt_bopb_h3_cfg{
    .symbol="tiausdt", .tag="TIA-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=45, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tiausdt_bopb_h3(s42_tiausdt_bopb_h3_cfg);
wire_engine(s42_tiausdt_bopb_h3);

// TON-BOPB-H4 minPF=2.52 score=14548
chimera::EdgeEngine::Config s42_tonusdt_bopb_h4_cfg{
    .symbol="tonusdt", .tag="TON-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_tonusdt_bopb_h4(s42_tonusdt_bopb_h4_cfg);
wire_engine(s42_tonusdt_bopb_h4);

// ZRO-KELT-H3 minPF=2.93 score=9753
chimera::EdgeEngine::Config s42_zrousdt_kelt_h3_cfg{
    .symbol="zrousdt", .tag="ZRO-KELT-H3", .kind=chimera::StrategyKind::KELTNER_REVERT,
    .tf_secs=10800, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_zrousdt_kelt_h3(s42_zrousdt_kelt_h3_cfg);
wire_engine(s42_zrousdt_kelt_h3);

// TRX-BOPB-H12 minPF=2.28 score=6746
chimera::EdgeEngine::Config s42_trxusdt_bopb_h12_cfg{
    .symbol="trxusdt", .tag="TRX-BOPB-H12", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=43200, .lookback=18, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s42_trxusdt_bopb_h12(s42_trxusdt_bopb_h12_cfg);
wire_engine(s42_trxusdt_bopb_h12);

// ─── PUSH_BACK BLOCKS ─────
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_tsmom_h4, "jtousdt", 14400, "JTO-TSMOM-H4", 2.41, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_tsmom_h3, "jtousdt", 10800, "JTO-TSMOM-H3", 2.25, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_tsmom_h8, "jtousdt", 28800, "JTO-TSMOM-H8", 2.71, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BOME, &s42_bomeusdt_tsmom_h4, "bomeusdt", 14400, "BOME-TSMOM-H4", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BOME, &s42_bomeusdt_tsmom_h3, "bomeusdt", 10800, "BOME-TSMOM-H3", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_tsmom_h12, "jtousdt", 43200, "JTO-TSMOM-H12", 2.46, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_INJ, &s42_injusdt_tsmom_h3, "injusdt", 10800, "INJ-TSMOM-H3", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FLOKI, &s42_flokiusdt_tsmom_h3, "flokiusdt", 10800, "FLOKI-TSMOM-H3", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_tsmom_h6, "jtousdt", 21600, "JTO-TSMOM-H6", 2.24, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_INJ, &s42_injusdt_tsmom_h4, "injusdt", 14400, "INJ-TSMOM-H4", 2.25, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ENA, &s42_enausdt_tsmom_h3, "enausdt", 10800, "ENA-TSMOM-H3", 2.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BOME, &s42_bomeusdt_tsmom_h6, "bomeusdt", 21600, "BOME-TSMOM-H6", 2.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_COMP, &s42_compusdt_tsmom_h3, "compusdt", 10800, "COMP-TSMOM-H3", 2.07, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ENA, &s42_enausdt_tsmom_h8, "enausdt", 28800, "ENA-TSMOM-H8", 2.48, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ICP, &s42_icpusdt_tsmom_h4, "icpusdt", 14400, "ICP-TSMOM-H4", 2.41, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_tsmom_h4, "tiausdt", 14400, "TIA-TSMOM-H4", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_WIF, &s42_wifusdt_tsmom_h6, "wifusdt", 21600, "WIF-TSMOM-H6", 2.22, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_COMP, &s42_compusdt_tsmom_h4, "compusdt", 14400, "COMP-TSMOM-H4", 2.14, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FET, &s42_fetusdt_tsmom_h6, "fetusdt", 21600, "FET-TSMOM-H6", 2.90, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FLOKI, &s42_flokiusdt_tsmom_h4, "flokiusdt", 14400, "FLOKI-TSMOM-H4", 2.21, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ENA, &s42_enausdt_tsmom_h6, "enausdt", 21600, "ENA-TSMOM-H6", 2.27, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BOME, &s42_bomeusdt_tsmom_h8, "bomeusdt", 28800, "BOME-TSMOM-H8", 2.26, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_INJ, &s42_injusdt_tsmom_h6, "injusdt", 21600, "INJ-TSMOM-H6", 2.42, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JUP, &s42_jupusdt_tsmom_h4, "jupusdt", 14400, "JUP-TSMOM-H4", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_UNI, &s42_uniusdt_tsmom_h4, "uniusdt", 14400, "UNI-TSMOM-H4", 2.17, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_PEPE, &s42_pepeusdt_tsmom_h4, "pepeusdt", 14400, "PEPE-TSMOM-H4", 2.11, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_WIF, &s42_wifusdt_tsmom_h8, "wifusdt", 28800, "WIF-TSMOM-H8", 2.09, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_IMX, &s42_imxusdt_tsmom_h4, "imxusdt", 14400, "IMX-TSMOM-H4", 2.34, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_tsmom_h6, "tiausdt", 21600, "TIA-TSMOM-H6", 2.06, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_COMP, &s42_compusdt_tsmom_h6, "compusdt", 21600, "COMP-TSMOM-H6", 2.35, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ETHFI, &s42_ethfiusdt_tsmom_h4, "ethfiusdt", 14400, "ETHFI-TSMOM-H4", 2.05, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FET, &s42_fetusdt_tsmom_h4, "fetusdt", 14400, "FET-TSMOM-H4", 2.29, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FLOKI, &s42_flokiusdt_tsmom_h6, "flokiusdt", 21600, "FLOKI-TSMOM-H6", 2.04, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ICP, &s42_icpusdt_tsmom_h8, "icpusdt", 28800, "ICP-TSMOM-H8", 2.43, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_WIF, &s42_wifusdt_tsmom_h12, "wifusdt", 43200, "WIF-TSMOM-H12", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SUI, &s42_suiusdt_tsmom_h3, "suiusdt", 10800, "SUI-TSMOM-H3", 2.13, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ICP, &s42_icpusdt_tsmom_h6, "icpusdt", 21600, "ICP-TSMOM-H6", 2.20, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_UNI, &s42_uniusdt_tsmom_h6, "uniusdt", 21600, "UNI-TSMOM-H6", 2.45, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BOME, &s42_bomeusdt_tsmom_d1, "bomeusdt", 86400, "BOME-TSMOM-D1", 2.62, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SEI, &s42_seiusdt_tsmom_h4, "seiusdt", 14400, "SEI-TSMOM-H4", 2.11, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_tsmom_h3, "tiausdt", 10800, "TIA-TSMOM-H3", 2.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_OP, &s42_opusdt_tsmom_h2, "opusdt", 7200, "OP-TSMOM-H2", 2.04, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_IMX, &s42_imxusdt_tsmom_h8, "imxusdt", 28800, "IMX-TSMOM-H8", 2.61, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_LDO, &s42_ldousdt_tsmom_h12, "ldousdt", 43200, "LDO-TSMOM-H12", 2.72, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ARB, &s42_arbusdt_tsmom_h4, "arbusdt", 14400, "ARB-TSMOM-H4", 2.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_COMP, &s42_compusdt_tsmom_h8, "compusdt", 28800, "COMP-TSMOM-H8", 2.43, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_tsmom_h3, "zrousdt", 10800, "ZRO-TSMOM-H3", 2.28, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_PYTH, &s42_pythusdt_tsmom_h6, "pythusdt", 21600, "PYTH-TSMOM-H6", 2.09, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ENA, &s42_enausdt_ichi_h2, "enausdt", 7200, "ENA-ICHI-H2", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_UNI, &s42_uniusdt_tsmom_h12, "uniusdt", 43200, "UNI-TSMOM-H12", 2.72, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_STX, &s42_stxusdt_tsmom_h6, "stxusdt", 21600, "STX-TSMOM-H6", 2.05, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ENA, &s42_enausdt_tsmom_h12, "enausdt", 43200, "ENA-TSMOM-H12", 2.36, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BOME, &s42_bomeusdt_tsmom_h12, "bomeusdt", 43200, "BOME-TSMOM-H12", 2.36, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FLOKI, &s42_flokiusdt_tsmom_h8, "flokiusdt", 28800, "FLOKI-TSMOM-H8", 2.21, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_LDO, &s42_ldousdt_tsmom_h8, "ldousdt", 28800, "LDO-TSMOM-H8", 2.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_UNI, &s42_uniusdt_tsmom_h8, "uniusdt", 28800, "UNI-TSMOM-H8", 2.30, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ARKM, &s42_arkmusdt_tsmom_h12, "arkmusdt", 43200, "ARKM-TSMOM-H12", 2.25, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_STX, &s42_stxusdt_tsmom_h8, "stxusdt", 28800, "STX-TSMOM-H8", 2.44, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_tsmom_h4, "zrousdt", 14400, "ZRO-TSMOM-H4", 2.72, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JUP, &s42_jupusdt_tsmom_h12, "jupusdt", 43200, "JUP-TSMOM-H12", 2.35, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_PEPE, &s42_pepeusdt_tsmom_h12, "pepeusdt", 43200, "PEPE-TSMOM-H12", 2.03, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TURBO, &s42_turbousdt_tsmom_h4, "turbousdt", 14400, "TURBO-TSMOM-H4", 2.07, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SUI, &s42_suiusdt_ichi_h3, "suiusdt", 10800, "SUI-ICHI-H3", 2.23, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_AVAX, &s42_avaxusdt_tsmom_h3, "avaxusdt", 10800, "AVAX-TSMOM-H3", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_COMP, &s42_compusdt_tsmom_h12, "compusdt", 43200, "COMP-TSMOM-H12", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_AAVE, &s42_aaveusdt_tsmom_h8, "aaveusdt", 28800, "AAVE-TSMOM-H8", 2.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_APT, &s42_aptusdt_tsmom_h4, "aptusdt", 14400, "APT-TSMOM-H4", 2.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_CRV, &s42_crvusdt_tsmom_h12, "crvusdt", 43200, "CRV-TSMOM-H12", 2.65, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JUP, &s42_jupusdt_tsmom_h8, "jupusdt", 28800, "JUP-TSMOM-H8", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_PYTH, &s42_pythusdt_tsmom_h12, "pythusdt", 43200, "PYTH-TSMOM-H12", 2.34, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_tsmom_h8, "eigenusdt", 28800, "EIGEN-TSMOM-H8", 2.82, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_PYTH, &s42_pythusdt_tsmom_h4, "pythusdt", 14400, "PYTH-TSMOM-H4", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_tsmom_h4, "eigenusdt", 14400, "EIGEN-TSMOM-H4", 2.34, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ETHFI, &s42_ethfiusdt_tsmom_h12, "ethfiusdt", 43200, "ETHFI-TSMOM-H12", 2.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_IMX, &s42_imxusdt_tsmom_h12, "imxusdt", 43200, "IMX-TSMOM-H12", 2.70, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_MANA, &s42_manausdt_tsmom_h4, "manausdt", 14400, "MANA-TSMOM-H4", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_tsmom_h6, "eigenusdt", 21600, "EIGEN-TSMOM-H6", 2.74, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_STX, &s42_stxusdt_tsmom_h12, "stxusdt", 43200, "STX-TSMOM-H12", 2.13, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_XLM, &s42_xlmusdt_tsmom_h4, "xlmusdt", 14400, "XLM-TSMOM-H4", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FET, &s42_fetusdt_ichi_h3, "fetusdt", 10800, "FET-ICHI-H3", 2.02, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_MASK, &s42_maskusdt_tsmom_h12, "maskusdt", 43200, "MASK-TSMOM-H12", 2.27, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_tsmom_d1, "zrousdt", 86400, "ZRO-TSMOM-D1", 2.38, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RUNE, &s42_runeusdt_tsmom_h4, "runeusdt", 14400, "RUNE-TSMOM-H4", 2.02, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_tsmom_h6, "zrousdt", 21600, "ZRO-TSMOM-H6", 2.02, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SAND, &s42_sandusdt_tsmom_h6, "sandusdt", 21600, "SAND-TSMOM-H6", 2.12, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ATOM, &s42_atomusdt_tsmom_h8, "atomusdt", 28800, "ATOM-TSMOM-H8", 2.78, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ETHFI, &s42_ethfiusdt_tsmom_d1, "ethfiusdt", 86400, "ETHFI-TSMOM-D1", 2.40, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ENA, &s42_enausdt_tsmom_d1, "enausdt", 86400, "ENA-TSMOM-D1", 3.05, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ICP, &s42_icpusdt_tsmom_h12, "icpusdt", 43200, "ICP-TSMOM-H12", 2.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ARKM, &s42_arkmusdt_tsmom_h8, "arkmusdt", 28800, "ARKM-TSMOM-H8", 2.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_tsmom_d1, "jtousdt", 86400, "JTO-TSMOM-D1", 2.25, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_tsmom_h3, "renderusdt", 10800, "RENDER-TSMOM-H3", 2.24, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_DOT, &s42_dotusdt_tsmom_h3, "dotusdt", 10800, "DOT-TSMOM-H3", 2.04, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ADA, &s42_adausdt_tsmom_h6, "adausdt", 21600, "ADA-TSMOM-H6", 2.17, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_GMT, &s42_gmtusdt_tsmom_h12, "gmtusdt", 43200, "GMT-TSMOM-H12", 2.13, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_tsmom_h12, "eigenusdt", 43200, "EIGEN-TSMOM-H12", 3.47, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SAND, &s42_sandusdt_tsmom_h8, "sandusdt", 28800, "SAND-TSMOM-H8", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TURBO, &s42_turbousdt_tsmom_h12, "turbousdt", 43200, "TURBO-TSMOM-H12", 2.38, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JUP, &s42_jupusdt_tsmom_d1, "jupusdt", 86400, "JUP-TSMOM-D1", 2.80, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_UNI, &s42_uniusdt_tsmom_d1, "uniusdt", 86400, "UNI-TSMOM-D1", 2.21, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ARB, &s42_arbusdt_tsmom_h12, "arbusdt", 43200, "ARB-TSMOM-H12", 2.29, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SOL, &s42_solusdt_tsmom_h8, "solusdt", 28800, "SOL-TSMOM-H8", 2.75, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FET, &s42_fetusdt_ichi_h4, "fetusdt", 14400, "FET-ICHI-H4", 2.55, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_ichi_h6, "jtousdt", 21600, "JTO-ICHI-H6", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_bopb_h4, "jtousdt", 14400, "JTO-BOPB-H4", 3.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_tsmom_h12, "renderusdt", 43200, "RENDER-TSMOM-H12", 4.39, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_W, &s42_wusdt_tsmom_h12, "wusdt", 43200, "W-TSMOM-H12", 2.26, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_DOT, &s42_dotusdt_tsmom_h6, "dotusdt", 21600, "DOT-TSMOM-H6", 2.17, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_ichi_h3, "tiausdt", 10800, "TIA-ICHI-H3", 2.09, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_XLM, &s42_xlmusdt_tsmom_h8, "xlmusdt", 28800, "XLM-TSMOM-H8", 2.46, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_INJ, &s42_injusdt_ichi_h4, "injusdt", 14400, "INJ-ICHI-H4", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_WIF, &s42_wifusdt_tsmom_d1, "wifusdt", 86400, "WIF-TSMOM-D1", 2.02, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_tsmom_h8, "renderusdt", 28800, "RENDER-TSMOM-H8", 2.38, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_IMX, &s42_imxusdt_tsmom_d1, "imxusdt", 86400, "IMX-TSMOM-D1", 2.25, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ARKM, &s42_arkmusdt_tsmom_d1, "arkmusdt", 86400, "ARKM-TSMOM-D1", 2.56, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ATOM, &s42_atomusdt_tsmom_h6, "atomusdt", 21600, "ATOM-TSMOM-H6", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RUNE, &s42_runeusdt_tsmom_h8, "runeusdt", 28800, "RUNE-TSMOM-H8", 2.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_MANA, &s42_manausdt_tsmom_h12, "manausdt", 43200, "MANA-TSMOM-H12", 2.03, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ATOM, &s42_atomusdt_tsmom_h12, "atomusdt", 43200, "ATOM-TSMOM-H12", 2.84, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_HBAR, &s42_hbarusdt_tsmom_h4, "hbarusdt", 14400, "HBAR-TSMOM-H4", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RUNE, &s42_runeusdt_tsmom_h12, "runeusdt", 43200, "RUNE-TSMOM-H12", 2.67, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ARB, &s42_arbusdt_tsmom_d1, "arbusdt", 86400, "ARB-TSMOM-D1", 3.58, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_DOT, &s42_dotusdt_tsmom_h12, "dotusdt", 43200, "DOT-TSMOM-H12", 2.13, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ICP, &s42_icpusdt_ichi_h4, "icpusdt", 14400, "ICP-ICHI-H4", 2.03, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_MANA, &s42_manausdt_tsmom_h8, "manausdt", 28800, "MANA-TSMOM-H8", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FIL, &s42_filusdt_tsmom_d1, "filusdt", 86400, "FIL-TSMOM-D1", 2.36, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_INJ, &s42_injusdt_ichi_h6, "injusdt", 21600, "INJ-ICHI-H6", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_XLM, &s42_xlmusdt_tsmom_h6, "xlmusdt", 21600, "XLM-TSMOM-H6", 2.07, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_MASK, &s42_maskusdt_tsmom_d1, "maskusdt", 86400, "MASK-TSMOM-D1", 2.34, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SUI, &s42_suiusdt_tsmom_d1, "suiusdt", 86400, "SUI-TSMOM-D1", 2.09, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_ichi_h4, "tiausdt", 14400, "TIA-ICHI-H4", 2.09, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_tsmom_h12, "zrousdt", 43200, "ZRO-TSMOM-H12", 2.74, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ETH, &s42_ethusdt_tsmom_h12, "ethusdt", 43200, "ETH-TSMOM-H12", 2.50, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_MASK, &s42_maskusdt_ichi_h3, "maskusdt", 10800, "MASK-ICHI-H3", 2.11, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SUI, &s42_suiusdt_ichi_h8, "suiusdt", 28800, "SUI-ICHI-H8", 2.65, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_ichi_h6, "tiausdt", 21600, "TIA-ICHI-H6", 2.44, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_OP, &s42_opusdt_tsmom_d1, "opusdt", 86400, "OP-TSMOM-D1", 2.23, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FLOKI, &s42_flokiusdt_tsmom_d1, "flokiusdt", 86400, "FLOKI-TSMOM-D1", 2.16, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_APT, &s42_aptusdt_tsmom_d1, "aptusdt", 86400, "APT-TSMOM-D1", 2.26, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_XLM, &s42_xlmusdt_ichi_h4, "xlmusdt", 14400, "XLM-ICHI-H4", 2.54, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_LTC, &s42_ltcusdt_tsmom_h12, "ltcusdt", 43200, "LTC-TSMOM-H12", 2.47, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_MANA, &s42_manausdt_tsmom_d1, "manausdt", 86400, "MANA-TSMOM-D1", 2.54, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_WIF, &s42_wifusdt_ichi_h6, "wifusdt", 21600, "WIF-ICHI-H6", 2.04, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_OP, &s42_opusdt_tsmom_h12, "opusdt", 43200, "OP-TSMOM-H12", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_ichi_h2, "renderusdt", 7200, "RENDER-ICHI-H2", 2.03, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FLOKI, &s42_flokiusdt_ichi_h8, "flokiusdt", 28800, "FLOKI-ICHI-H8", 2.07, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_tsmom_d1, "eigenusdt", 86400, "EIGEN-TSMOM-D1", 3.62, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_STX, &s42_stxusdt_tsmom_d1, "stxusdt", 86400, "STX-TSMOM-D1", 2.06, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_tsmom_d2, "zrousdt", 172800, "ZRO-TSMOM-D2", 6.35, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_CRV, &s42_crvusdt_tsmom_d1, "crvusdt", 86400, "CRV-TSMOM-D1", 2.35, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ONDO, &s42_ondousdt_tsmom_h4, "ondousdt", 14400, "ONDO-TSMOM-H4", 2.17, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BCH, &s42_bchusdt_tsmom_d1, "bchusdt", 86400, "BCH-TSMOM-D1", 2.74, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_ichi_h3, "renderusdt", 10800, "RENDER-ICHI-H3", 2.14, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_ichi_h3, "zrousdt", 10800, "ZRO-ICHI-H3", 2.03, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_AVAX, &s42_avaxusdt_tsmom_d1, "avaxusdt", 86400, "AVAX-TSMOM-D1", 2.42, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_JTO, &s42_jtousdt_ichi_h8, "jtousdt", 28800, "JTO-ICHI-H8", 2.06, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_ichi_h4, "zrousdt", 14400, "ZRO-ICHI-H4", 2.35, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SHIB, &s42_shibusdt_tsmom_h8, "shibusdt", 28800, "SHIB-TSMOM-H8", 2.01, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SHIB, &s42_shibusdt_tsmom_h12, "shibusdt", 43200, "SHIB-TSMOM-H12", 2.14, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RUNE, &s42_runeusdt_tsmom_d1, "runeusdt", 86400, "RUNE-TSMOM-D1", 2.12, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ATOM, &s42_atomusdt_ichi_h4, "atomusdt", 14400, "ATOM-ICHI-H4", 2.25, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_XLM, &s42_xlmusdt_tsmom_d1, "xlmusdt", 86400, "XLM-TSMOM-D1", 2.05, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ATOM, &s42_atomusdt_tsmom_d2, "atomusdt", 172800, "ATOM-TSMOM-D2", 4.28, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TON, &s42_tonusdt_tsmom_h8, "tonusdt", 28800, "TON-TSMOM-H8", 2.14, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_BCH, &s42_bchusdt_tsmom_h12, "bchusdt", 43200, "BCH-TSMOM-H12", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FLOKI, &s42_flokiusdt_ichi_h12, "flokiusdt", 43200, "FLOKI-ICHI-H12", 2.08, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_DOGE, &s42_dogeusdt_ichi_h8, "dogeusdt", 28800, "DOGE-ICHI-H8", 2.26, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_OP, &s42_opusdt_tsmom_d2, "opusdt", 172800, "OP-TSMOM-D2", 2.29, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RUNE, &s42_runeusdt_ichi_h8, "runeusdt", 28800, "RUNE-ICHI-H8", 2.77, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_OP, &s42_opusdt_ichi_h8, "opusdt", 28800, "OP-ICHI-H8", 2.06, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ONDO, &s42_ondousdt_tsmom_h6, "ondousdt", 21600, "ONDO-TSMOM-H6", 2.29, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SOL, &s42_solusdt_ichi_h6, "solusdt", 21600, "SOL-ICHI-H6", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_XLM, &s42_xlmusdt_ichi_h8, "xlmusdt", 28800, "XLM-ICHI-H8", 2.05, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ONDO, &s42_ondousdt_ichi_h3, "ondousdt", 10800, "ONDO-ICHI-H3", 2.32, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_tsmom_d2, "renderusdt", 172800, "RENDER-TSMOM-D2", 3.19, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_tsmom_d1, "tiausdt", 86400, "TIA-TSMOM-D1", 3.33, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ETC, &s42_etcusdt_tsmom_d1, "etcusdt", 86400, "ETC-TSMOM-D1", 2.26, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RUNE, &s42_runeusdt_tsmom_d2, "runeusdt", 172800, "RUNE-TSMOM-D2", 2.08, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ATOM, &s42_atomusdt_tsmom_d1, "atomusdt", 86400, "ATOM-TSMOM-D1", 2.22, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TRX, &s42_trxusdt_tsmom_h12, "trxusdt", 43200, "TRX-TSMOM-H12", 2.08, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ONDO, &s42_ondousdt_tsmom_d1, "ondousdt", 86400, "ONDO-TSMOM-D1", 2.50, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TON, &s42_tonusdt_tsmom_h6, "tonusdt", 21600, "TON-TSMOM-H6", 2.14, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_ichi_d2, "zrousdt", 172800, "ZRO-ICHI-D2", 3.53, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_ichi_h8, "zrousdt", 28800, "ZRO-ICHI-H8", 2.97, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_VET, &s42_vetusdt_tsmom_d1, "vetusdt", 86400, "VET-TSMOM-D1", 2.10, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_LTC, &s42_ltcusdt_tsmom_d1, "ltcusdt", 86400, "LTC-TSMOM-D1", 2.25, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TRX, &s42_trxusdt_tsmom_d3, "trxusdt", 259200, "TRX-TSMOM-D3", 2.87, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_LTC, &s42_ltcusdt_ichi_h12, "ltcusdt", 43200, "LTC-ICHI-H12", 3.11, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_willr_h6, "zrousdt", 21600, "ZRO-WILLR-H6", 2.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_ichi_h12, "eigenusdt", 43200, "EIGEN-ICHI-H12", 3.40, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_dch_h8, "eigenusdt", 28800, "EIGEN-DCH-H8", 3.19, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_sr_h12, "eigenusdt", 43200, "EIGEN-SR-H12", 3.42, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_ichi_h12, "renderusdt", 43200, "RENDER-ICHI-H12", 2.00, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RUNE, &s42_runeusdt_ichi_h6, "runeusdt", 21600, "RUNE-ICHI-H6", 2.06, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SUI, &s42_suiusdt_bopb_h4, "suiusdt", 14400, "SUI-BOPB-H4", 2.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TON, &s42_tonusdt_tsmom_d1, "tonusdt", 86400, "TON-TSMOM-D1", 2.43, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_ichi_h6, "eigenusdt", 21600, "EIGEN-ICHI-H6", 2.02, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ONDO, &s42_ondousdt_ichi_h4, "ondousdt", 14400, "ONDO-ICHI-H4", 2.30, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_ichi_h12, "zrousdt", 43200, "ZRO-ICHI-H12", 2.09, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_bopb_h6, "tiausdt", 21600, "TIA-BOPB-H6", 2.02, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_EIGEN, &s42_eigenusdt_ichi_h8, "eigenusdt", 28800, "EIGEN-ICHI-H8", 2.29, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_UNI, &s42_uniusdt_boll_h2, "uniusdt", 7200, "UNI-BOLL-H2", 2.31, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_willr_h12, "renderusdt", 43200, "RENDER-WILLR-H12", 2.83, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_FET, &s42_fetusdt_bopb_h3, "fetusdt", 10800, "FET-BOPB-H3", 2.02, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ONDO, &s42_ondousdt_dt_h12, "ondousdt", 43200, "ONDO-DT-H12", 2.11, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_SUI, &s42_suiusdt_bopb_h6, "suiusdt", 21600, "SUI-BOPB-H6", 2.03, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ONDO, &s42_ondousdt_ichi_h12, "ondousdt", 43200, "ONDO-ICHI-H12", 2.37, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_RENDER, &s42_renderusdt_bopb_h8, "renderusdt", 28800, "RENDER-BOPB-H8", 2.42, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TURBO, &s42_turbousdt_kelt_h4, "turbousdt", 14400, "TURBO-KELT-H4", 4.15, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TIA, &s42_tiausdt_bopb_h3, "tiausdt", 10800, "TIA-BOPB-H3", 2.53, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TON, &s42_tonusdt_bopb_h4, "tonusdt", 14400, "TON-BOPB-H4", 2.52, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_ZRO, &s42_zrousdt_kelt_h3, "zrousdt", 10800, "ZRO-KELT-H3", 2.93, 0.0, 100, 730, 42});
g_slots.push_back({chimera::SYM_TRX, &s42_trxusdt_bopb_h12, "trxusdt", 43200, "TRX-BOPB-H12", 2.28, 0.0, 100, 730, 42});
