// S39 ADDS — 218 new engines from ultra-wide --discover (31 sym × 10 TF × 144 grid)
// All survive PF>=1.5 + net_bp>0 in ALL 4 WF windows (134/180/365/730d) under prod_tiered_pyramid_xlow.
// Tag collisions with main.cpp + engines_s38_new.cpp dropped.
// ─── CONFIG + ENGINE + WIRE ─────────────────────────────────────────────
// 220163|jupusdt,H1,TSMOM,45,24,1.0|min PF=1.58|134d 1.79 n=924|180d 1.70|365d 1.58|730d 1.70
chimera::EdgeEngine::Config s39_jupusdt_tsmom_h1_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_h1(s39_jupusdt_tsmom_h1_cfg);
wire_engine(s39_jupusdt_tsmom_h1);

// 215240|enausdt,H2,TSMOM,60,12,1.0|min PF=1.82|134d 2.58 n=364|180d 2.02|365d 1.82|730d 1.90
chimera::EdgeEngine::Config s39_enausdt_tsmom_h2_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_tsmom_h2(s39_enausdt_tsmom_h2_cfg);
wire_engine(s39_enausdt_tsmom_h2);

// 211913|fetusdt,H2,TSMOM,60,24,1.0|min PF=1.83|134d 2.32 n=453|180d 1.83|365d 2.06|730d 1.94
chimera::EdgeEngine::Config s39_fetusdt_tsmom_h2_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_tsmom_h2(s39_fetusdt_tsmom_h2_cfg);
wire_engine(s39_fetusdt_tsmom_h2);

// 199704|injusdt,H3,TSMOM,60,8,1.0|min PF=2.10|134d 2.65 n=371|180d 2.32|365d 2.10|730d 2.12
chimera::EdgeEngine::Config s39_injusdt_tsmom_h3_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_tsmom_h3(s39_injusdt_tsmom_h3_cfg);
wire_engine(s39_injusdt_tsmom_h3);

// 197398|wifusdt,H3,TSMOM,30,18,1.0|min PF=1.63|134d 1.63 n=313|180d 1.78|365d 1.94|730d 2.07
chimera::EdgeEngine::Config s39_wifusdt_tsmom_h3_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_wifusdt_tsmom_h3(s39_wifusdt_tsmom_h3_cfg);
wire_engine(s39_wifusdt_tsmom_h3);

// 194190|jupusdt,H2,TSMOM,45,5,1.0|min PF=1.80|134d 1.87 n=514|180d 1.80|365d 1.92|730d 1.99
chimera::EdgeEngine::Config s39_jupusdt_tsmom_h2_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_h2(s39_jupusdt_tsmom_h2_cfg);
wire_engine(s39_jupusdt_tsmom_h2);

// 193430|injusdt,H2,TSMOM,60,24,1.0|min PF=1.63|134d 1.80 n=531|180d 1.63|365d 1.81|730d 2.02
chimera::EdgeEngine::Config s39_injusdt_tsmom_h2_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_tsmom_h2(s39_injusdt_tsmom_h2_cfg);
wire_engine(s39_injusdt_tsmom_h2);

// 189139|enausdt,H3,TSMOM,45,5,1.0|min PF=1.82|134d 1.82 n=234|180d 1.84|365d 2.07|730d 1.98
chimera::EdgeEngine::Config s39_enausdt_tsmom_h3_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_tsmom_h3(s39_enausdt_tsmom_h3_cfg);
wire_engine(s39_enausdt_tsmom_h3);

// 188299|fetusdt,H3,TSMOM,45,3,1.0|min PF=1.90|134d 2.17 n=305|180d 1.90|365d 1.99|730d 2.07
chimera::EdgeEngine::Config s39_fetusdt_tsmom_h3_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_tsmom_h3(s39_fetusdt_tsmom_h3_cfg);
wire_engine(s39_fetusdt_tsmom_h3);

// 185680|injusdt,H4,TSMOM,60,24,1.0|min PF=2.25|134d 2.59 n=295|180d 2.34|365d 2.25|730d 2.50
chimera::EdgeEngine::Config s39_injusdt_tsmom_h4_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_tsmom_h4(s39_injusdt_tsmom_h4_cfg);
wire_engine(s39_injusdt_tsmom_h4);

// 183066|jupusdt,H3,TSMOM,30,3,1.0|min PF=1.90|134d 1.97 n=358|180d 1.90|365d 1.94|730d 2.16
chimera::EdgeEngine::Config s39_jupusdt_tsmom_h3_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_h3(s39_jupusdt_tsmom_h3_cfg);
wire_engine(s39_jupusdt_tsmom_h3);

// 182761|wifusdt,H2,TSMOM,60,5,2.0|min PF=1.62|134d 1.62 n=351|180d 1.76|365d 1.81|730d 1.98
chimera::EdgeEngine::Config s39_wifusdt_tsmom_h2_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_wifusdt_tsmom_h2(s39_wifusdt_tsmom_h2_cfg);
wire_engine(s39_wifusdt_tsmom_h2);

// 175832|tiausdt,H2,TSMOM,60,5,1.0|min PF=1.58|134d 1.70 n=487|180d 1.58|365d 1.62|730d 1.97
chimera::EdgeEngine::Config s39_tiausdt_tsmom_h2_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_tsmom_h2(s39_tiausdt_tsmom_h2_cfg);
wire_engine(s39_tiausdt_tsmom_h2);

// 172693|enausdt,H4,TSMOM,45,8,1.0|min PF=1.94|134d 2.38 n=175|180d 2.60|365d 2.19|730d 1.94
chimera::EdgeEngine::Config s39_enausdt_tsmom_h4_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_tsmom_h4(s39_enausdt_tsmom_h4_cfg);
wire_engine(s39_enausdt_tsmom_h4);

// 170237|suiusdt,H3,TSMOM,60,24,1.0|min PF=1.83|134d 2.19 n=322|180d 2.14|365d 1.83|730d 2.25
chimera::EdgeEngine::Config s39_suiusdt_tsmom_h3_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_tsmom_h3(s39_suiusdt_tsmom_h3_cfg);
wire_engine(s39_suiusdt_tsmom_h3);

// 168306|ldousdt,H2,TSMOM,60,5,1.0|min PF=1.68|134d 1.75 n=416|180d 1.68|365d 1.76|730d 1.82
chimera::EdgeEngine::Config s39_ldousdt_tsmom_h2_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_h2(s39_ldousdt_tsmom_h2_cfg);
wire_engine(s39_ldousdt_tsmom_h2);

// 168160|enausdt,H8,TSMOM,45,3,1.0|min PF=2.48|134d 3.18 n=105|180d 3.26|365d 2.69|730d 2.48
chimera::EdgeEngine::Config s39_enausdt_tsmom_h8_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_tsmom_h8(s39_enausdt_tsmom_h8_cfg);
wire_engine(s39_enausdt_tsmom_h8);

// 168050|tiausdt,H3,TSMOM,30,24,1.0|min PF=1.66|134d 1.83 n=326|180d 1.66|365d 1.94|730d 2.03
chimera::EdgeEngine::Config s39_tiausdt_tsmom_h3_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_tsmom_h3(s39_tiausdt_tsmom_h3_cfg);
wire_engine(s39_tiausdt_tsmom_h3);

// 165759|suiusdt,H2,TSMOM,45,24,1.0|min PF=1.69|134d 1.96 n=471|180d 1.88|365d 1.69|730d 1.78
chimera::EdgeEngine::Config s39_suiusdt_tsmom_h2_cfg{
    .symbol="suiusdt", .tag="SUI-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_tsmom_h2(s39_suiusdt_tsmom_h2_cfg);
wire_engine(s39_suiusdt_tsmom_h2);

// 165662|wifusdt,H6,TSMOM,60,18,1.0|min PF=2.22|134d 3.14 n=149|180d 3.19|365d 2.58|730d 2.22
chimera::EdgeEngine::Config s39_wifusdt_tsmom_h6_cfg{
    .symbol="wifusdt", .tag="WIF-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_wifusdt_tsmom_h6(s39_wifusdt_tsmom_h6_cfg);
wire_engine(s39_wifusdt_tsmom_h6);

// 164947|fetusdt,H6,TSMOM,60,12,2.0|min PF=2.90|134d 4.22 n=130|180d 3.66|365d 2.90|730d 3.03
chimera::EdgeEngine::Config s39_fetusdt_tsmom_h6_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_tsmom_h6(s39_fetusdt_tsmom_h6_cfg);
wire_engine(s39_fetusdt_tsmom_h6);

// 164716|seiusdt,H2,TSMOM,60,12,1.0|min PF=1.62|134d 1.63 n=393|180d 1.62|365d 1.90|730d 2.05
chimera::EdgeEngine::Config s39_seiusdt_tsmom_h2_cfg{
    .symbol="seiusdt", .tag="SEI-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_seiusdt_tsmom_h2(s39_seiusdt_tsmom_h2_cfg);
wire_engine(s39_seiusdt_tsmom_h2);

// 162618|enausdt,H6,TSMOM,60,5,1.0|min PF=2.27|134d 2.53 n=138|180d 2.48|365d 2.27|730d 2.31
chimera::EdgeEngine::Config s39_enausdt_tsmom_h6_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_tsmom_h6(s39_enausdt_tsmom_h6_cfg);
wire_engine(s39_enausdt_tsmom_h6);

// 160867|pepeusdt,H3,TSMOM,45,5,1.0|min PF=1.79|134d 1.83 n=339|180d 1.79|365d 1.97|730d 1.96
chimera::EdgeEngine::Config s39_pepeusdt_tsmom_h3_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_pepeusdt_tsmom_h3(s39_pepeusdt_tsmom_h3_cfg);
wire_engine(s39_pepeusdt_tsmom_h3);

// 159728|injusdt,H6,TSMOM,30,24,1.0|min PF=2.42|134d 3.40 n=190|180d 2.62|365d 2.43|730d 2.42
chimera::EdgeEngine::Config s39_injusdt_tsmom_h6_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_tsmom_h6(s39_injusdt_tsmom_h6_cfg);
wire_engine(s39_injusdt_tsmom_h6);

// 159604|jupusdt,H4,TSMOM,60,5,1.0|min PF=1.90|134d 2.11 n=272|180d 2.06|365d 1.90|730d 2.25
chimera::EdgeEngine::Config s39_jupusdt_tsmom_h4_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_h4(s39_jupusdt_tsmom_h4_cfg);
wire_engine(s39_jupusdt_tsmom_h4);

// 156889|uniusdt,H4,TSMOM,60,5,1.0|min PF=2.17|134d 2.17 n=207|180d 2.39|365d 2.42|730d 2.31
chimera::EdgeEngine::Config s39_uniusdt_tsmom_h4_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_tsmom_h4(s39_uniusdt_tsmom_h4_cfg);
wire_engine(s39_uniusdt_tsmom_h4);

// 156868|pepeusdt,H6,TSMOM,45,12,1.0|min PF=2.06|134d 2.22 n=193|180d 2.33|365d 2.06|730d 2.27
chimera::EdgeEngine::Config s39_pepeusdt_tsmom_h6_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_pepeusdt_tsmom_h6(s39_pepeusdt_tsmom_h6_cfg);
wire_engine(s39_pepeusdt_tsmom_h6);

// 156540|ldousdt,H1,TSMOM,60,12,4.0|min PF=1.53|134d 1.53 n=548|180d 1.54|365d 1.80|730d 1.70
chimera::EdgeEngine::Config s39_ldousdt_tsmom_h1_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_h1(s39_ldousdt_tsmom_h1_cfg);
wire_engine(s39_ldousdt_tsmom_h1);

// 149880|arbusdt,H3,TSMOM,45,12,1.0|min PF=1.93|134d 2.33 n=318|180d 2.00|365d 1.93|730d 1.93
chimera::EdgeEngine::Config s39_arbusdt_tsmom_h3_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_arbusdt_tsmom_h3(s39_arbusdt_tsmom_h3_cfg);
wire_engine(s39_arbusdt_tsmom_h3);

// 149745|tiausdt,H6,TSMOM,30,8,1.0|min PF=2.06|134d 2.34 n=175|180d 2.20|365d 2.30|730d 2.06
chimera::EdgeEngine::Config s39_tiausdt_tsmom_h6_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_tsmom_h6(s39_tiausdt_tsmom_h6_cfg);
wire_engine(s39_tiausdt_tsmom_h6);

// 149287|arbusdt,H2,TSMOM,45,8,1.0|min PF=1.75|134d 1.92 n=432|180d 1.80|365d 1.77|730d 1.75
chimera::EdgeEngine::Config s39_arbusdt_tsmom_h2_cfg{
    .symbol="arbusdt", .tag="ARB-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_arbusdt_tsmom_h2(s39_arbusdt_tsmom_h2_cfg);
wire_engine(s39_arbusdt_tsmom_h2);

// 149092|aaveusdt,H3,TSMOM,45,5,1.0|min PF=1.89|134d 2.04 n=263|180d 1.89|365d 1.99|730d 2.03
chimera::EdgeEngine::Config s39_aaveusdt_tsmom_h3_cfg{
    .symbol="aaveusdt", .tag="AAVE-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_tsmom_h3(s39_aaveusdt_tsmom_h3_cfg);
wire_engine(s39_aaveusdt_tsmom_h3);

// 148590|aaveusdt,H2,TSMOM,60,5,1.0|min PF=1.72|134d 1.72 n=365|180d 1.90|365d 1.83|730d 1.84
chimera::EdgeEngine::Config s39_aaveusdt_tsmom_h2_cfg{
    .symbol="aaveusdt", .tag="AAVE-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_tsmom_h2(s39_aaveusdt_tsmom_h2_cfg);
wire_engine(s39_aaveusdt_tsmom_h2);

// 147052|ldousdt,H4,TSMOM,45,5,2.0|min PF=1.96|134d 2.00 n=149|180d 1.96|365d 2.32|730d 2.43
chimera::EdgeEngine::Config s39_ldousdt_tsmom_h4_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_h4(s39_ldousdt_tsmom_h4_cfg);
wire_engine(s39_ldousdt_tsmom_h4);

// 146233|opusdt,H2,TSMOM,60,8,1.0|min PF=1.76|134d 1.98 n=428|180d 1.76|365d 1.82|730d 1.77
chimera::EdgeEngine::Config s39_opusdt_tsmom_h2_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_h2(s39_opusdt_tsmom_h2_cfg);
wire_engine(s39_opusdt_tsmom_h2);

// 145688|maticusdt,H3,TSMOM,60,5,1.0|min PF=1.90|134d 2.16 n=255|180d 1.90|365d 2.24|730d 2.25
chimera::EdgeEngine::Config s39_maticusdt_tsmom_h3_cfg{
    .symbol="maticusdt", .tag="MATIC-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_tsmom_h3(s39_maticusdt_tsmom_h3_cfg);
wire_engine(s39_maticusdt_tsmom_h3);

// 144524|hbarusdt,H2,TSMOM,60,3,1.0|min PF=1.56|134d 1.71 n=429|180d 1.56|365d 1.74|730d 1.96
chimera::EdgeEngine::Config s39_hbarusdt_tsmom_h2_cfg{
    .symbol="hbarusdt", .tag="HBAR-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_hbarusdt_tsmom_h2(s39_hbarusdt_tsmom_h2_cfg);
wire_engine(s39_hbarusdt_tsmom_h2);

// 143358|ldousdt,H3,TSMOM,60,18,2.0|min PF=1.60|134d 1.88 n=204|180d 1.60|365d 2.00|730d 2.28
chimera::EdgeEngine::Config s39_ldousdt_tsmom_h3_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_h3(s39_ldousdt_tsmom_h3_cfg);
wire_engine(s39_ldousdt_tsmom_h3);

// 143351|uniusdt,H3,TSMOM,45,5,1.0|min PF=1.51|134d 1.56 n=290|180d 1.51|365d 2.04|730d 1.89
chimera::EdgeEngine::Config s39_uniusdt_tsmom_h3_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_tsmom_h3(s39_uniusdt_tsmom_h3_cfg);
wire_engine(s39_uniusdt_tsmom_h3);

// 141274|uniusdt,H6,TSMOM,60,8,1.0|min PF=2.45|134d 3.00 n=140|180d 2.78|365d 2.45|730d 2.52
chimera::EdgeEngine::Config s39_uniusdt_tsmom_h6_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_tsmom_h6(s39_uniusdt_tsmom_h6_cfg);
wire_engine(s39_uniusdt_tsmom_h6);

// 139728|seiusdt,H4,TSMOM,60,5,1.0|min PF=2.11|134d 2.11 n=174|180d 2.21|365d 2.31|730d 2.33
chimera::EdgeEngine::Config s39_seiusdt_tsmom_h4_cfg{
    .symbol="seiusdt", .tag="SEI-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_seiusdt_tsmom_h4(s39_seiusdt_tsmom_h4_cfg);
wire_engine(s39_seiusdt_tsmom_h4);

// 139346|filusdt,H1,TSMOM,45,18,2.0|min PF=1.51|134d 1.83 n=604|180d 1.82|365d 1.57|730d 1.51
chimera::EdgeEngine::Config s39_filusdt_tsmom_h1_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=45, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_h1(s39_filusdt_tsmom_h1_cfg);
wire_engine(s39_filusdt_tsmom_h1);

// 137160|filusdt,H3,TSMOM,45,5,1.0|min PF=1.67|134d 2.14 n=318|180d 1.67|365d 2.01|730d 1.77
chimera::EdgeEngine::Config s39_filusdt_tsmom_h3_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_h3(s39_filusdt_tsmom_h3_cfg);
wire_engine(s39_filusdt_tsmom_h3);

// 136280|jupusdt,H6,TSMOM,45,3,1.0|min PF=1.88|134d 1.90 n=174|180d 1.88|365d 2.22|730d 2.22
chimera::EdgeEngine::Config s39_jupusdt_tsmom_h6_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_h6(s39_jupusdt_tsmom_h6_cfg);
wire_engine(s39_jupusdt_tsmom_h6);

// 135772|filusdt,H4,TSMOM,60,3,1.0|min PF=1.97|134d 1.97 n=235|180d 2.07|365d 2.09|730d 2.10
chimera::EdgeEngine::Config s39_filusdt_tsmom_h4_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_h4(s39_filusdt_tsmom_h4_cfg);
wire_engine(s39_filusdt_tsmom_h4);

// 135738|nearusdt,H2,TSMOM,60,24,1.0|min PF=1.51|134d 1.65 n=517|180d 1.51|365d 1.56|730d 1.69
chimera::EdgeEngine::Config s39_nearusdt_tsmom_h2_cfg{
    .symbol="nearusdt", .tag="NEAR-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_nearusdt_tsmom_h2(s39_nearusdt_tsmom_h2_cfg);
wire_engine(s39_nearusdt_tsmom_h2);

// 133835|opusdt,H3,TSMOM,45,24,1.0|min PF=1.81|134d 2.29 n=308|180d 1.95|365d 1.81|730d 1.94
chimera::EdgeEngine::Config s39_opusdt_tsmom_h3_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_h3(s39_opusdt_tsmom_h3_cfg);
wire_engine(s39_opusdt_tsmom_h3);

// 133657|enausdt,H2,ICHIMOKU,6,12,1.0|min PF=1.88|134d 2.26 n=197|180d 1.92|365d 1.91|730d 1.88
chimera::EdgeEngine::Config s39_enausdt_ichi_h2_cfg{
    .symbol="enausdt", .tag="ENA-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_ichi_h2(s39_enausdt_ichi_h2_cfg);
wire_engine(s39_enausdt_ichi_h2);

// 132820|opusdt,H1,TSMOM,60,18,2.0|min PF=1.52|134d 1.54 n=601|180d 1.52|365d 1.57|730d 1.58
chimera::EdgeEngine::Config s39_opusdt_tsmom_h1_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_h1(s39_opusdt_tsmom_h1_cfg);
wire_engine(s39_opusdt_tsmom_h1);

// 132114|ldousdt,H12,TSMOM,60,8,1.0|min PF=2.72|134d 3.52 n=77|180d 3.68|365d 3.19|730d 2.72
chimera::EdgeEngine::Config s39_ldousdt_tsmom_h12_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_h12(s39_ldousdt_tsmom_h12_cfg);
wire_engine(s39_ldousdt_tsmom_h12);

// 131915|maticusdt,H4,TSMOM,45,5,1.0|min PF=2.10|134d 2.34 n=183|180d 2.30|365d 2.54|730d 2.10
chimera::EdgeEngine::Config s39_maticusdt_tsmom_h4_cfg{
    .symbol="maticusdt", .tag="MATIC-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_tsmom_h4(s39_maticusdt_tsmom_h4_cfg);
wire_engine(s39_maticusdt_tsmom_h4);

// 131106|aptusdt,H2,TSMOM,60,8,1.0|min PF=1.52|134d 1.52 n=448|180d 1.52|365d 1.70|730d 1.72
chimera::EdgeEngine::Config s39_aptusdt_tsmom_h2_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aptusdt_tsmom_h2(s39_aptusdt_tsmom_h2_cfg);
wire_engine(s39_aptusdt_tsmom_h2);

// 129120|enausdt,H12,TSMOM,45,3,1.0|min PF=1.96|134d 2.67 n=67|180d 1.96|365d 2.94|730d 2.82
chimera::EdgeEngine::Config s39_enausdt_tsmom_h12_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_tsmom_h12(s39_enausdt_tsmom_h12_cfg);
wire_engine(s39_enausdt_tsmom_h12);

// 127728|uniusdt,H12,TSMOM,30,5,1.0|min PF=2.72|134d 5.88 n=76|180d 4.81|365d 2.72|730d 2.83
chimera::EdgeEngine::Config s39_uniusdt_tsmom_h12_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_tsmom_h12(s39_uniusdt_tsmom_h12_cfg);
wire_engine(s39_uniusdt_tsmom_h12);

// 126024|jupusdt,H8,TSMOM,12,5,1.0|min PF=1.82|134d 4.19 n=139|180d 3.15|365d 2.34|730d 1.82
chimera::EdgeEngine::Config s39_jupusdt_tsmom_h8_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=12, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_h8(s39_jupusdt_tsmom_h8_cfg);
wire_engine(s39_jupusdt_tsmom_h8);

// 125068|avaxusdt,H3,TSMOM,60,8,1.0|min PF=1.52|134d 1.52 n=333|180d 1.62|365d 1.86|730d 2.13
chimera::EdgeEngine::Config s39_avaxusdt_tsmom_h3_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_avaxusdt_tsmom_h3(s39_avaxusdt_tsmom_h3_cfg);
wire_engine(s39_avaxusdt_tsmom_h3);

// 124742|dogeusdt,H2,TSMOM,60,3,1.0|min PF=1.53|134d 1.75 n=478|180d 1.53|365d 1.57|730d 1.75
chimera::EdgeEngine::Config s39_dogeusdt_tsmom_h2_cfg{
    .symbol="dogeusdt", .tag="DOGE-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_dogeusdt_tsmom_h2(s39_dogeusdt_tsmom_h2_cfg);
wire_engine(s39_dogeusdt_tsmom_h2);

// 124682|seiusdt,H6,TSMOM,45,3,1.0|min PF=1.55|134d 1.83 n=113|180d 1.55|365d 2.06|730d 2.77
chimera::EdgeEngine::Config s39_seiusdt_tsmom_h6_cfg{
    .symbol="seiusdt", .tag="SEI-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_seiusdt_tsmom_h6(s39_seiusdt_tsmom_h6_cfg);
wire_engine(s39_seiusdt_tsmom_h6);

// 123471|ldousdt,H8,TSMOM,45,24,1.0|min PF=2.15|134d 2.35 n=106|180d 2.32|365d 2.54|730d 2.15
chimera::EdgeEngine::Config s39_ldousdt_tsmom_h8_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_h8(s39_ldousdt_tsmom_h8_cfg);
wire_engine(s39_ldousdt_tsmom_h8);

// 122060|uniusdt,H8,TSMOM,60,5,2.0|min PF=2.30|134d 3.25 n=93|180d 2.99|365d 2.30|730d 3.97
chimera::EdgeEngine::Config s39_uniusdt_tsmom_h8_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_tsmom_h8(s39_uniusdt_tsmom_h8_cfg);
wire_engine(s39_uniusdt_tsmom_h8);

// 118092|jupusdt,H12,TSMOM,30,3,1.0|min PF=2.35|134d 2.40 n=102|180d 2.50|365d 2.71|730d 2.35
chimera::EdgeEngine::Config s39_jupusdt_tsmom_h12_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_h12(s39_jupusdt_tsmom_h12_cfg);
wire_engine(s39_jupusdt_tsmom_h12);

// 117214|adausdt,H3,TSMOM,60,5,1.0|min PF=1.58|134d 1.61 n=278|180d 1.58|365d 1.93|730d 2.11
chimera::EdgeEngine::Config s39_adausdt_tsmom_h3_cfg{
    .symbol="adausdt", .tag="ADA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_adausdt_tsmom_h3(s39_adausdt_tsmom_h3_cfg);
wire_engine(s39_adausdt_tsmom_h3);

// 116533|hbarusdt,H4,TSMOM,45,3,1.0|min PF=1.77|134d 1.77 n=201|180d 1.89|365d 1.82|730d 2.19
chimera::EdgeEngine::Config s39_hbarusdt_tsmom_h4_cfg{
    .symbol="hbarusdt", .tag="HBAR-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_hbarusdt_tsmom_h4(s39_hbarusdt_tsmom_h4_cfg);
wire_engine(s39_hbarusdt_tsmom_h4);

// 116440|suiusdt,H3,ICHIMOKU,45,24,1.0|min PF=2.23|134d 2.35 n=169|180d 2.45|365d 2.36|730d 2.23
chimera::EdgeEngine::Config s39_suiusdt_ichi_h3_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_ichi_h3(s39_suiusdt_ichi_h3_cfg);
wire_engine(s39_suiusdt_ichi_h3);

// 115272|jupusdt,H1,ICHIMOKU,60,24,1.0|min PF=1.53|134d 1.70 n=487|180d 1.58|365d 1.57|730d 1.53
chimera::EdgeEngine::Config s39_jupusdt_ichi_h1_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_ichi_h1(s39_jupusdt_ichi_h1_cfg);
wire_engine(s39_jupusdt_ichi_h1);

// 114634|filusdt,H2,TSMOM,45,18,3.0|min PF=1.57|134d 1.61 n=316|180d 2.03|365d 1.57|730d 1.78
chimera::EdgeEngine::Config s39_filusdt_tsmom_h2_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_h2(s39_filusdt_tsmom_h2_cfg);
wire_engine(s39_filusdt_tsmom_h2);

// 113371|opusdt,H4,TSMOM,60,8,1.0|min PF=1.87|134d 2.00 n=214|180d 1.87|365d 1.87|730d 1.99
chimera::EdgeEngine::Config s39_opusdt_tsmom_h4_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_h4(s39_opusdt_tsmom_h4_cfg);
wire_engine(s39_opusdt_tsmom_h4);

// 112859|maticusdt,H8,TSMOM,60,5,1.0|min PF=1.98|134d 2.23 n=82|180d 1.98|365d 3.12|730d 2.91
chimera::EdgeEngine::Config s39_maticusdt_tsmom_h8_cfg{
    .symbol="maticusdt", .tag="MATIC-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_tsmom_h8(s39_maticusdt_tsmom_h8_cfg);
wire_engine(s39_maticusdt_tsmom_h8);

// 112813|aptusdt,H3,TSMOM,45,3,1.0|min PF=1.55|134d 1.55 n=323|180d 1.71|365d 1.64|730d 1.74
chimera::EdgeEngine::Config s39_aptusdt_tsmom_h3_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aptusdt_tsmom_h3(s39_aptusdt_tsmom_h3_cfg);
wire_engine(s39_aptusdt_tsmom_h3);

// 110155|aaveusdt,H8,TSMOM,60,5,1.0|min PF=2.16|134d 2.33 n=78|180d 2.16|365d 2.21|730d 2.48
chimera::EdgeEngine::Config s39_aaveusdt_tsmom_h8_cfg{
    .symbol="aaveusdt", .tag="AAVE-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_tsmom_h8(s39_aaveusdt_tsmom_h8_cfg);
wire_engine(s39_aaveusdt_tsmom_h8);

// 109199|enausdt,H3,ICHIMOKU,30,24,2.0|min PF=1.71|134d 2.31 n=92|180d 1.71|365d 2.79|730d 2.08
chimera::EdgeEngine::Config s39_enausdt_ichi_h3_cfg{
    .symbol="enausdt", .tag="ENA-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_ichi_h3(s39_enausdt_ichi_h3_cfg);
wire_engine(s39_enausdt_ichi_h3);

// 102787|pepeusdt,H3,ICHIMOKU,18,5,1.0|min PF=1.72|134d 1.72 n=167|180d 1.81|365d 2.06|730d 1.96
chimera::EdgeEngine::Config s39_pepeusdt_ichi_h3_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_pepeusdt_ichi_h3(s39_pepeusdt_ichi_h3_cfg);
wire_engine(s39_pepeusdt_ichi_h3);

// 102711|atomusdt,H3,TSMOM,60,5,1.0|min PF=1.51|134d 1.51 n=338|180d 1.60|365d 1.70|730d 2.01
chimera::EdgeEngine::Config s39_atomusdt_tsmom_h3_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_tsmom_h3(s39_atomusdt_tsmom_h3_cfg);
wire_engine(s39_atomusdt_tsmom_h3);

// 102229|maticusdt,H6,TSMOM,30,3,1.0|min PF=2.01|134d 2.01 n=132|180d 2.15|365d 2.31|730d 2.11
chimera::EdgeEngine::Config s39_maticusdt_tsmom_h6_cfg{
    .symbol="maticusdt", .tag="MATIC-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_tsmom_h6(s39_maticusdt_tsmom_h6_cfg);
wire_engine(s39_maticusdt_tsmom_h6);

// 101525|uniusdt,H2,TSMOM,12,24,3.0|min PF=1.51|134d 1.51 n=304|180d 1.60|365d 1.64|730d 1.54
chimera::EdgeEngine::Config s39_uniusdt_tsmom_h2_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=12, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_tsmom_h2(s39_uniusdt_tsmom_h2_cfg);
wire_engine(s39_uniusdt_tsmom_h2);

// 101008|injusdt,H3,ICHIMOKU,60,5,1.0|min PF=1.99|134d 2.52 n=176|180d 2.36|365d 2.04|730d 1.99
chimera::EdgeEngine::Config s39_injusdt_ichi_h3_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_ichi_h3(s39_injusdt_ichi_h3_cfg);
wire_engine(s39_injusdt_ichi_h3);

// 100521|tiausdt,H2,ICHIMOKU,12,12,1.0|min PF=1.62|134d 1.93 n=243|180d 1.75|365d 1.62|730d 1.77
chimera::EdgeEngine::Config s39_tiausdt_ichi_h2_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=12, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_ichi_h2(s39_tiausdt_ichi_h2_cfg);
wire_engine(s39_tiausdt_ichi_h2);

// 99561|fetusdt,H3,ICHIMOKU,6,24,3.0|min PF=2.02|134d 4.69 n=115|180d 3.21|365d 2.02|730d 2.25
chimera::EdgeEngine::Config s39_fetusdt_ichi_h3_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_ichi_h3(s39_fetusdt_ichi_h3_cfg);
wire_engine(s39_fetusdt_ichi_h3);

// 99090|ldousdt,D1,TSMOM,60,3,1.0|min PF=3.29|134d 3.61 n=29|180d 3.61|365d 3.29|730d 3.29
chimera::EdgeEngine::Config s39_ldousdt_tsmom_d1_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_d1(s39_ldousdt_tsmom_d1_cfg);
wire_engine(s39_ldousdt_tsmom_d1);

// 99046|opusdt,H8,TSMOM,18,24,1.0|min PF=1.90|134d 1.96 n=116|180d 2.10|365d 1.90|730d 2.14
chimera::EdgeEngine::Config s39_opusdt_tsmom_h8_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_h8(s39_opusdt_tsmom_h8_cfg);
wire_engine(s39_opusdt_tsmom_h8);

// 99039|atomusdt,H4,TSMOM,60,3,1.0|min PF=1.92|134d 1.99 n=261|180d 2.07|365d 1.92|730d 2.12
chimera::EdgeEngine::Config s39_atomusdt_tsmom_h4_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_tsmom_h4(s39_atomusdt_tsmom_h4_cfg);
wire_engine(s39_atomusdt_tsmom_h4);

// 98868|ldousdt,H6,TSMOM,60,3,2.0|min PF=1.90|134d 1.96 n=121|180d 1.90|365d 2.09|730d 2.02
chimera::EdgeEngine::Config s39_ldousdt_tsmom_h6_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_h6(s39_ldousdt_tsmom_h6_cfg);
wire_engine(s39_ldousdt_tsmom_h6);

// 98313|filusdt,H12,TSMOM,18,3,1.0|min PF=1.85|134d 3.55 n=89|180d 2.76|365d 2.19|730d 1.85
chimera::EdgeEngine::Config s39_filusdt_tsmom_h12_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_h12(s39_filusdt_tsmom_h12_cfg);
wire_engine(s39_filusdt_tsmom_h12);

// 98097|linkusdt,H2,TSMOM,60,5,2.0|min PF=1.52|134d 1.67 n=413|180d 1.52|365d 1.64|730d 1.70
chimera::EdgeEngine::Config s39_linkusdt_tsmom_h2_cfg{
    .symbol="linkusdt", .tag="LINK-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_linkusdt_tsmom_h2(s39_linkusdt_tsmom_h2_cfg);
wire_engine(s39_linkusdt_tsmom_h2);

// 98011|jupusdt,H2,ICHIMOKU,45,5,1.0|min PF=1.69|134d 2.01 n=238|180d 1.87|365d 1.69|730d 1.73
chimera::EdgeEngine::Config s39_jupusdt_ichi_h2_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_ichi_h2(s39_jupusdt_ichi_h2_cfg);
wire_engine(s39_jupusdt_ichi_h2);

// 97496|atomusdt,H8,TSMOM,60,12,1.0|min PF=2.78|134d 2.79 n=145|180d 3.01|365d 2.91|730d 2.78
chimera::EdgeEngine::Config s39_atomusdt_tsmom_h8_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_tsmom_h8(s39_atomusdt_tsmom_h8_cfg);
wire_engine(s39_atomusdt_tsmom_h8);

// 96999|enausdt,D1,TSMOM,30,3,2.0|min PF=3.05|134d 4.07 n=27|180d 6.57|365d 4.22|730d 3.05
chimera::EdgeEngine::Config s39_enausdt_tsmom_d1_cfg{
    .symbol="enausdt", .tag="ENA-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_tsmom_d1(s39_enausdt_tsmom_d1_cfg);
wire_engine(s39_enausdt_tsmom_d1);

// 96743|hbarusdt,H6,TSMOM,45,12,1.0|min PF=1.70|134d 1.70 n=130|180d 2.00|365d 1.82|730d 2.31
chimera::EdgeEngine::Config s39_hbarusdt_tsmom_h6_cfg{
    .symbol="hbarusdt", .tag="HBAR-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_hbarusdt_tsmom_h6(s39_hbarusdt_tsmom_h6_cfg);
wire_engine(s39_hbarusdt_tsmom_h6);

// 96002|filusdt,H8,TSMOM,6,8,1.0|min PF=1.84|134d 1.92 n=115|180d 1.86|365d 2.01|730d 1.84
chimera::EdgeEngine::Config s39_filusdt_tsmom_h8_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=6, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_h8(s39_filusdt_tsmom_h8_cfg);
wire_engine(s39_filusdt_tsmom_h8);

// 95338|fetusdt,H2,ICHIMOKU,30,12,1.0|min PF=1.55|134d 1.78 n=242|180d 1.55|365d 1.70|730d 1.75
chimera::EdgeEngine::Config s39_fetusdt_ichi_h2_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=30, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_ichi_h2(s39_fetusdt_ichi_h2_cfg);
wire_engine(s39_fetusdt_ichi_h2);

// 94356|adausdt,H4,TSMOM,30,3,1.0|min PF=1.51|134d 1.51 n=221|180d 1.57|365d 1.67|730d 2.00
chimera::EdgeEngine::Config s39_adausdt_tsmom_h4_cfg{
    .symbol="adausdt", .tag="ADA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_adausdt_tsmom_h4(s39_adausdt_tsmom_h4_cfg);
wire_engine(s39_adausdt_tsmom_h4);

// 93543|adausdt,H6,TSMOM,45,3,2.0|min PF=2.17|134d 2.34 n=129|180d 3.32|365d 2.17|730d 2.25
chimera::EdgeEngine::Config s39_adausdt_tsmom_h6_cfg{
    .symbol="adausdt", .tag="ADA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_adausdt_tsmom_h6(s39_adausdt_tsmom_h6_cfg);
wire_engine(s39_adausdt_tsmom_h6);

// 92712|wifusdt,H3,ICHIMOKU,30,12,1.0|min PF=1.65|134d 1.82 n=100|180d 1.65|365d 1.88|730d 1.98
chimera::EdgeEngine::Config s39_wifusdt_ichi_h3_cfg{
    .symbol="wifusdt", .tag="WIF-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_wifusdt_ichi_h3(s39_wifusdt_ichi_h3_cfg);
wire_engine(s39_wifusdt_ichi_h3);

// 91908|jupusdt,D1,TSMOM,6,12,3.0|min PF=2.80|134d 3.62 n=36|180d 534.30|365d 5.26|730d 2.80
chimera::EdgeEngine::Config s39_jupusdt_tsmom_d1_cfg{
    .symbol="jupusdt", .tag="JUP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_tsmom_d1(s39_jupusdt_tsmom_d1_cfg);
wire_engine(s39_jupusdt_tsmom_d1);

// 91078|injusdt,H4,ICHIMOKU,30,24,1.0|min PF=1.95|134d 2.63 n=119|180d 2.49|365d 1.95|730d 2.15
chimera::EdgeEngine::Config s39_injusdt_ichi_h4_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_ichi_h4(s39_injusdt_ichi_h4_cfg);
wire_engine(s39_injusdt_ichi_h4);

// 91056|uniusdt,D1,TSMOM,12,3,1.0|min PF=2.21|134d 3.37 n=39|180d 3.35|365d 2.71|730d 2.21
chimera::EdgeEngine::Config s39_uniusdt_tsmom_d1_cfg{
    .symbol="uniusdt", .tag="UNI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_tsmom_d1(s39_uniusdt_tsmom_d1_cfg);
wire_engine(s39_uniusdt_tsmom_d1);

// 90922|injusdt,H2,ICHIMOKU,12,24,4.0|min PF=1.75|134d 1.98 n=181|180d 1.75|365d 1.82|730d 2.19
chimera::EdgeEngine::Config s39_injusdt_ichi_h2_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_ichi_h2(s39_injusdt_ichi_h2_cfg);
wire_engine(s39_injusdt_ichi_h2);

// 89353|ondousdt,H2,TSMOM,45,24,1.0|min PF=1.77|134d 2.09 n=471|180d 2.01|365d 1.77|730d 2.06
chimera::EdgeEngine::Config s39_ondousdt_tsmom_h2_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ondousdt_tsmom_h2(s39_ondousdt_tsmom_h2_cfg);
wire_engine(s39_ondousdt_tsmom_h2);

// 89267|filusdt,H6,TSMOM,60,3,1.0|min PF=1.69|134d 2.26 n=154|180d 1.95|365d 1.69|730d 1.84
chimera::EdgeEngine::Config s39_filusdt_tsmom_h6_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_h6(s39_filusdt_tsmom_h6_cfg);
wire_engine(s39_filusdt_tsmom_h6);

// 87900|opusdt,H6,TSMOM,45,5,1.0|min PF=1.63|134d 1.86 n=155|180d 1.79|365d 1.63|730d 1.87
chimera::EdgeEngine::Config s39_opusdt_tsmom_h6_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_h6(s39_opusdt_tsmom_h6_cfg);
wire_engine(s39_opusdt_tsmom_h6);

// 87112|tiausdt,H3,ICHIMOKU,6,8,2.0|min PF=2.09|134d 2.50 n=140|180d 2.15|365d 2.47|730d 2.09
chimera::EdgeEngine::Config s39_tiausdt_ichi_h3_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_ichi_h3(s39_tiausdt_ichi_h3_cfg);
wire_engine(s39_tiausdt_ichi_h3);

// 87038|suiusdt,H2,ICHIMOKU,18,8,1.0|min PF=1.53|134d 1.64 n=250|180d 1.57|365d 1.53|730d 1.59
chimera::EdgeEngine::Config s39_suiusdt_ichi_h2_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=18, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_ichi_h2(s39_suiusdt_ichi_h2_cfg);
wire_engine(s39_suiusdt_ichi_h2);

// 86000|ondousdt,H3,TSMOM,60,3,1.0|min PF=1.84|134d 2.15 n=352|180d 1.84|365d 2.00|730d 2.23
chimera::EdgeEngine::Config s39_ondousdt_tsmom_h3_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ondousdt_tsmom_h3(s39_ondousdt_tsmom_h3_cfg);
wire_engine(s39_ondousdt_tsmom_h3);

// 84800|aaveusdt,H3,ICHIMOKU,45,3,1.0|min PF=1.87|134d 2.31 n=127|180d 2.40|365d 1.97|730d 1.87
chimera::EdgeEngine::Config s39_aaveusdt_ichi_h3_cfg{
    .symbol="aaveusdt", .tag="AAVE-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_ichi_h3(s39_aaveusdt_ichi_h3_cfg);
wire_engine(s39_aaveusdt_ichi_h3);

// 84635|atomusdt,H6,TSMOM,45,3,1.0|min PF=1.99|134d 2.22 n=193|180d 2.34|365d 1.99|730d 2.12
chimera::EdgeEngine::Config s39_atomusdt_tsmom_h6_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_tsmom_h6(s39_atomusdt_tsmom_h6_cfg);
wire_engine(s39_atomusdt_tsmom_h6);

// 84305|maticusdt,H12,TSMOM,45,5,1.0|min PF=2.30|134d 2.93 n=46|180d 3.48|365d 2.30|730d 2.69
chimera::EdgeEngine::Config s39_maticusdt_tsmom_h12_cfg{
    .symbol="maticusdt", .tag="MATIC-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_tsmom_h12(s39_maticusdt_tsmom_h12_cfg);
wire_engine(s39_maticusdt_tsmom_h12);

// 84020|wifusdt,H2,ICHIMOKU,30,18,4.0|min PF=1.50|134d 1.50 n=86|180d 1.89|365d 1.55|730d 1.95
chimera::EdgeEngine::Config s39_wifusdt_ichi_h2_cfg{
    .symbol="wifusdt", .tag="WIF-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=30, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_wifusdt_ichi_h2(s39_wifusdt_ichi_h2_cfg);
wire_engine(s39_wifusdt_ichi_h2);

// 81055|enausdt,H4,ICHIMOKU,6,24,1.0|min PF=1.76|134d 2.05 n=79|180d 2.45|365d 1.76|730d 1.77
chimera::EdgeEngine::Config s39_enausdt_ichi_h4_cfg{
    .symbol="enausdt", .tag="ENA-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=6, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_enausdt_ichi_h4(s39_enausdt_ichi_h4_cfg);
wire_engine(s39_enausdt_ichi_h4);

// 79960|aaveusdt,H2,ICHIMOKU,6,5,1.0|min PF=1.60|134d 1.83 n=177|180d 1.93|365d 1.60|730d 1.65
chimera::EdgeEngine::Config s39_aaveusdt_ichi_h2_cfg{
    .symbol="aaveusdt", .tag="AAVE-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_ichi_h2(s39_aaveusdt_ichi_h2_cfg);
wire_engine(s39_aaveusdt_ichi_h2);

// 79628|xrpusdt,H8,TSMOM,30,8,1.0|min PF=1.59|134d 1.83 n=129|180d 1.59|365d 2.06|730d 2.17
chimera::EdgeEngine::Config s39_xrpusdt_tsmom_h8_cfg{
    .symbol="xrpusdt", .tag="XRP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_xrpusdt_tsmom_h8(s39_xrpusdt_tsmom_h8_cfg);
wire_engine(s39_xrpusdt_tsmom_h8);

// 79333|atomusdt,H12,TSMOM,30,3,2.0|min PF=2.84|134d 2.88 n=87|180d 3.68|365d 3.58|730d 2.84
chimera::EdgeEngine::Config s39_atomusdt_tsmom_h12_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_tsmom_h12(s39_atomusdt_tsmom_h12_cfg);
wire_engine(s39_atomusdt_tsmom_h12);

// 78791|aaveusdt,H12,TSMOM,45,3,4.0|min PF=1.66|134d 1.66 n=38|180d 3.24|365d 2.20|730d 2.77
chimera::EdgeEngine::Config s39_aaveusdt_tsmom_h12_cfg{
    .symbol="aaveusdt", .tag="AAVE-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_tsmom_h12(s39_aaveusdt_tsmom_h12_cfg);
wire_engine(s39_aaveusdt_tsmom_h12);

// 77821|ldousdt,H1,ICHIMOKU,18,24,4.0|min PF=1.53|134d 1.56 n=275|180d 1.54|365d 1.59|730d 1.53
chimera::EdgeEngine::Config s39_ldousdt_ichi_h1_cfg{
    .symbol="ldousdt", .tag="LDO-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_ichi_h1(s39_ldousdt_ichi_h1_cfg);
wire_engine(s39_ldousdt_ichi_h1);

// 77606|filusdt,D1,TSMOM,18,24,3.0|min PF=2.36|134d 5.35 n=28|180d 2.49|365d 3.71|730d 2.36
chimera::EdgeEngine::Config s39_filusdt_tsmom_d1_cfg{
    .symbol="filusdt", .tag="FIL-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_tsmom_d1(s39_filusdt_tsmom_d1_cfg);
wire_engine(s39_filusdt_tsmom_d1);

// 76791|injusdt,H6,ICHIMOKU,18,24,1.0|min PF=2.01|134d 2.47 n=88|180d 2.48|365d 2.31|730d 2.01
chimera::EdgeEngine::Config s39_injusdt_ichi_h6_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_ichi_h6(s39_injusdt_ichi_h6_cfg);
wire_engine(s39_injusdt_ichi_h6);

// 76465|opusdt,H12,TSMOM,18,3,2.0|min PF=1.78|134d 1.81 n=67|180d 1.78|365d 2.20|730d 2.26
chimera::EdgeEngine::Config s39_opusdt_tsmom_h12_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_h12(s39_opusdt_tsmom_h12_cfg);
wire_engine(s39_opusdt_tsmom_h12);

// 75658|arbusdt,H2,ICHIMOKU,30,24,2.0|min PF=1.62|134d 1.66 n=211|180d 1.62|365d 1.75|730d 1.75
chimera::EdgeEngine::Config s39_arbusdt_ichi_h2_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=30, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_arbusdt_ichi_h2(s39_arbusdt_ichi_h2_cfg);
wire_engine(s39_arbusdt_ichi_h2);

// 75447|opusdt,H2,ICHIMOKU,6,8,2.0|min PF=1.69|134d 1.83 n=180|180d 1.89|365d 1.87|730d 1.69
chimera::EdgeEngine::Config s39_opusdt_ichi_h2_cfg{
    .symbol="opusdt", .tag="OP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_ichi_h2(s39_opusdt_ichi_h2_cfg);
wire_engine(s39_opusdt_ichi_h2);

// 74696|injusdt,H8,ICHIMOKU,6,18,3.0|min PF=2.94|134d 11.14 n=59|180d 5.55|365d 5.67|730d 2.94
chimera::EdgeEngine::Config s39_injusdt_ichi_h8_cfg{
    .symbol="injusdt", .tag="INJ-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_ichi_h8(s39_injusdt_ichi_h8_cfg);
wire_engine(s39_injusdt_ichi_h8);

// 74283|jupusdt,H3,ICHIMOKU,6,3,1.0|min PF=1.52|134d 1.52 n=151|180d 1.54|365d 1.67|730d 1.91
chimera::EdgeEngine::Config s39_jupusdt_ichi_h3_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_ichi_h3(s39_jupusdt_ichi_h3_cfg);
wire_engine(s39_jupusdt_ichi_h3);

// 73560|tiausdt,H6,ICHIMOKU,18,12,1.0|min PF=1.89|134d 3.14 n=83|180d 2.88|365d 2.60|730d 1.89
chimera::EdgeEngine::Config s39_tiausdt_ichi_h6_cfg{
    .symbol="tiausdt", .tag="TIA-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_ichi_h6(s39_tiausdt_ichi_h6_cfg);
wire_engine(s39_tiausdt_ichi_h6);

// 73440|nearusdt,H3,ICHIMOKU,60,24,4.0|min PF=2.04|134d 2.88 n=138|180d 2.81|365d 2.31|730d 2.04
chimera::EdgeEngine::Config s39_nearusdt_ichi_h3_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_nearusdt_ichi_h3(s39_nearusdt_ichi_h3_cfg);
wire_engine(s39_nearusdt_ichi_h3);

// 72526|seiusdt,H2,ICHIMOKU,12,12,4.0|min PF=1.60|134d 1.68 n=128|180d 1.60|365d 1.95|730d 1.86
chimera::EdgeEngine::Config s39_seiusdt_ichi_h2_cfg{
    .symbol="seiusdt", .tag="SEI-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_seiusdt_ichi_h2(s39_seiusdt_ichi_h2_cfg);
wire_engine(s39_seiusdt_ichi_h2);

// 71768|atomusdt,H3,ICHIMOKU,45,5,1.0|min PF=1.73|134d 1.77 n=181|180d 1.73|365d 1.89|730d 2.15
chimera::EdgeEngine::Config s39_atomusdt_ichi_h3_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_ichi_h3(s39_atomusdt_ichi_h3_cfg);
wire_engine(s39_atomusdt_ichi_h3);

// 71104|wifusdt,H6,ICHIMOKU,30,18,1.0|min PF=1.80|134d 3.87 n=44|180d 3.38|365d 1.80|730d 1.91
chimera::EdgeEngine::Config s39_wifusdt_ichi_h6_cfg{
    .symbol="wifusdt", .tag="WIF-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_wifusdt_ichi_h6(s39_wifusdt_ichi_h6_cfg);
wire_engine(s39_wifusdt_ichi_h6);

// 71031|arbusdt,H3,ICHIMOKU,18,5,1.0|min PF=1.51|134d 1.70 n=173|180d 1.51|365d 1.66|730d 1.76
chimera::EdgeEngine::Config s39_arbusdt_ichi_h3_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_arbusdt_ichi_h3(s39_arbusdt_ichi_h3_cfg);
wire_engine(s39_arbusdt_ichi_h3);

// 70968|opusdt,D1,TSMOM,12,18,1.0|min PF=2.23|134d 2.23 n=38|180d 2.26|365d 2.26|730d 2.77
chimera::EdgeEngine::Config s39_opusdt_tsmom_d1_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_d1(s39_opusdt_tsmom_d1_cfg);
wire_engine(s39_opusdt_tsmom_d1);

// 68597|dogeusdt,H3,ICHIMOKU,30,8,1.0|min PF=1.54|134d 1.58 n=152|180d 1.54|365d 1.78|730d 1.85
chimera::EdgeEngine::Config s39_dogeusdt_ichi_h3_cfg{
    .symbol="dogeusdt", .tag="DOGE-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_dogeusdt_ichi_h3(s39_dogeusdt_ichi_h3_cfg);
wire_engine(s39_dogeusdt_ichi_h3);

// 68377|maticusdt,H3,ICHIMOKU,18,5,3.0|min PF=1.65|134d 1.78 n=112|180d 1.65|365d 1.98|730d 2.25
chimera::EdgeEngine::Config s39_maticusdt_ichi_h3_cfg{
    .symbol="maticusdt", .tag="MATIC-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=5, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_ichi_h3(s39_maticusdt_ichi_h3_cfg);
wire_engine(s39_maticusdt_ichi_h3);

// 68198|pepeusdt,D2,TSMOM,18,3,1.0|min PF=2.52|134d 4.18 n=23|180d 4.99|365d 4.45|730d 2.52
chimera::EdgeEngine::Config s39_pepeusdt_tsmom_d2_cfg{
    .symbol="pepeusdt", .tag="PEPE-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_pepeusdt_tsmom_d2(s39_pepeusdt_tsmom_d2_cfg);
wire_engine(s39_pepeusdt_tsmom_d2);

// 67336|seiusdt,H4,ICHIMOKU,18,5,1.0|min PF=1.70|134d 1.70 n=79|180d 1.77|365d 1.92|730d 1.95
chimera::EdgeEngine::Config s39_seiusdt_ichi_h4_cfg{
    .symbol="seiusdt", .tag="SEI-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_seiusdt_ichi_h4(s39_seiusdt_ichi_h4_cfg);
wire_engine(s39_seiusdt_ichi_h4);

// 66150|maticusdt,D1,TSMOM,6,5,1.0|min PF=2.39|134d 5.08 n=37|180d 5.00|365d 2.39|730d 2.40
chimera::EdgeEngine::Config s39_maticusdt_tsmom_d1_cfg{
    .symbol="maticusdt", .tag="MATIC-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_tsmom_d1(s39_maticusdt_tsmom_d1_cfg);
wire_engine(s39_maticusdt_tsmom_d1);

// 65439|jupusdt,H4,ICHIMOKU,12,12,1.0|min PF=1.82|134d 1.82 n=110|180d 1.95|365d 1.82|730d 1.86
chimera::EdgeEngine::Config s39_jupusdt_ichi_h4_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=12, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_ichi_h4(s39_jupusdt_ichi_h4_cfg);
wire_engine(s39_jupusdt_ichi_h4);

// 63662|opusdt,H3,ICHIMOKU,30,12,2.0|min PF=1.71|134d 2.26 n=107|180d 2.29|365d 1.91|730d 1.71
chimera::EdgeEngine::Config s39_opusdt_ichi_h3_cfg{
    .symbol="opusdt", .tag="OP-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_ichi_h3(s39_opusdt_ichi_h3_cfg);
wire_engine(s39_opusdt_ichi_h3);

// 62962|suiusdt,H6,ICHIMOKU,12,12,2.0|min PF=1.64|134d 1.81 n=68|180d 1.64|365d 1.98|730d 2.11
chimera::EdgeEngine::Config s39_suiusdt_ichi_h6_cfg{
    .symbol="suiusdt", .tag="SUI-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=12, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_ichi_h6(s39_suiusdt_ichi_h6_cfg);
wire_engine(s39_suiusdt_ichi_h6);

// 62461|maticusdt,H4,ICHIMOKU,30,24,4.0|min PF=1.99|134d 1.99 n=81|180d 2.23|365d 3.46|730d 2.32
chimera::EdgeEngine::Config s39_maticusdt_ichi_h4_cfg{
    .symbol="maticusdt", .tag="MATIC-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_maticusdt_ichi_h4(s39_maticusdt_ichi_h4_cfg);
wire_engine(s39_maticusdt_ichi_h4);

// 61775|uniusdt,H2,ICHIMOKU,45,5,4.0|min PF=1.51|134d 1.51 n=132|180d 1.54|365d 1.52|730d 1.68
chimera::EdgeEngine::Config s39_uniusdt_ichi_h2_cfg{
    .symbol="uniusdt", .tag="UNI-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_ichi_h2(s39_uniusdt_ichi_h2_cfg);
wire_engine(s39_uniusdt_ichi_h2);

// 61108|avaxusdt,H3,ICHIMOKU,6,8,2.0|min PF=1.70|134d 2.46 n=141|180d 1.70|365d 1.91|730d 1.97
chimera::EdgeEngine::Config s39_avaxusdt_ichi_h3_cfg{
    .symbol="avaxusdt", .tag="AVAX-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_avaxusdt_ichi_h3(s39_avaxusdt_ichi_h3_cfg);
wire_engine(s39_avaxusdt_ichi_h3);

// 60714|pepeusdt,H6,ICHIMOKU,18,18,1.0|min PF=1.60|134d 1.60 n=90|180d 1.70|365d 1.68|730d 1.76
chimera::EdgeEngine::Config s39_pepeusdt_ichi_h6_cfg{
    .symbol="pepeusdt", .tag="PEPE-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_pepeusdt_ichi_h6(s39_pepeusdt_ichi_h6_cfg);
wire_engine(s39_pepeusdt_ichi_h6);

// 60446|hbarusdt,H6,ICHIMOKU,18,12,1.0|min PF=1.81|134d 2.78 n=63|180d 1.81|365d 1.97|730d 2.23
chimera::EdgeEngine::Config s39_hbarusdt_ichi_h6_cfg{
    .symbol="hbarusdt", .tag="HBAR-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_hbarusdt_ichi_h6(s39_hbarusdt_ichi_h6_cfg);
wire_engine(s39_hbarusdt_ichi_h6);

// 58512|hbarusdt,H4,ICHIMOKU,60,24,4.0|min PF=1.95|134d 2.00 n=54|180d 1.95|365d 2.65|730d 2.87
chimera::EdgeEngine::Config s39_hbarusdt_ichi_h4_cfg{
    .symbol="hbarusdt", .tag="HBAR-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_hbarusdt_ichi_h4(s39_hbarusdt_ichi_h4_cfg);
wire_engine(s39_hbarusdt_ichi_h4);

// 58162|nearusdt,H6,ICHIMOKU,45,5,2.0|min PF=1.98|134d 2.06 n=107|180d 2.04|365d 1.98|730d 2.18
chimera::EdgeEngine::Config s39_nearusdt_ichi_h6_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_nearusdt_ichi_h6(s39_nearusdt_ichi_h6_cfg);
wire_engine(s39_nearusdt_ichi_h6);

// 58006|aaveusdt,D1,TSMOM,12,12,3.0|min PF=1.67|134d 1.92 n=28|180d 2.88|365d 1.67|730d 2.82
chimera::EdgeEngine::Config s39_aaveusdt_tsmom_d1_cfg{
    .symbol="aaveusdt", .tag="AAVE-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_tsmom_d1(s39_aaveusdt_tsmom_d1_cfg);
wire_engine(s39_aaveusdt_tsmom_d1);

// 57589|nearusdt,H2,ICHIMOKU,30,18,4.0|min PF=1.67|134d 1.79 n=171|180d 1.72|365d 1.67|730d 1.68
chimera::EdgeEngine::Config s39_nearusdt_ichi_h2_cfg{
    .symbol="nearusdt", .tag="NEAR-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=30, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_nearusdt_ichi_h2(s39_nearusdt_ichi_h2_cfg);
wire_engine(s39_nearusdt_ichi_h2);

// 57217|jupusdt,H6,ICHIMOKU,18,3,1.0|min PF=1.67|134d 1.99 n=83|180d 1.91|365d 1.82|730d 1.67
chimera::EdgeEngine::Config s39_jupusdt_ichi_h6_cfg{
    .symbol="jupusdt", .tag="JUP-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_ichi_h6(s39_jupusdt_ichi_h6_cfg);
wire_engine(s39_jupusdt_ichi_h6);

// 55889|aptusdt,H3,ICHIMOKU,60,8,4.0|min PF=1.76|134d 2.05 n=108|180d 2.06|365d 2.07|730d 1.76
chimera::EdgeEngine::Config s39_aptusdt_ichi_h3_cfg{
    .symbol="aptusdt", .tag="APT-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=60, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aptusdt_ichi_h3(s39_aptusdt_ichi_h3_cfg);
wire_engine(s39_aptusdt_ichi_h3);

// 55736|atomusdt,H4,ICHIMOKU,18,12,4.0|min PF=2.25|134d 2.56 n=100|180d 3.49|365d 2.25|730d 2.35
chimera::EdgeEngine::Config s39_atomusdt_ichi_h4_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=18, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_ichi_h4(s39_atomusdt_ichi_h4_cfg);
wire_engine(s39_atomusdt_ichi_h4);

// 55569|atomusdt,D1,TSMOM,45,5,1.0|min PF=1.92|134d 1.92 n=48|180d 2.45|365d 2.54|730d 3.13
chimera::EdgeEngine::Config s39_atomusdt_tsmom_d1_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_tsmom_d1(s39_atomusdt_tsmom_d1_cfg);
wire_engine(s39_atomusdt_tsmom_d1);

// 54480|ldousdt,D2,TSMOM,12,24,1.0|min PF=2.32|134d 2.88 n=21|180d 3.17|365d 2.57|730d 2.32
chimera::EdgeEngine::Config s39_ldousdt_tsmom_d2_cfg{
    .symbol="ldousdt", .tag="LDO-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_tsmom_d2(s39_ldousdt_tsmom_d2_cfg);
wire_engine(s39_ldousdt_tsmom_d2);

// 54375|ldousdt,H3,ICHIMOKU,45,8,2.0|min PF=1.68|134d 2.00 n=62|180d 1.77|365d 1.68|730d 1.70
chimera::EdgeEngine::Config s39_ldousdt_ichi_h3_cfg{
    .symbol="ldousdt", .tag="LDO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_ichi_h3(s39_ldousdt_ichi_h3_cfg);
wire_engine(s39_ldousdt_ichi_h3);

// 53768|dogeusdt,H6,ICHIMOKU,60,18,3.0|min PF=1.93|134d 2.85 n=52|180d 2.85|365d 1.93|730d 2.82
chimera::EdgeEngine::Config s39_dogeusdt_ichi_h6_cfg{
    .symbol="dogeusdt", .tag="DOGE-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_dogeusdt_ichi_h6(s39_dogeusdt_ichi_h6_cfg);
wire_engine(s39_dogeusdt_ichi_h6);

// 53583|aptusdt,D2,TSMOM,12,3,1.0|min PF=2.57|134d 3.97 n=24|180d 3.58|365d 2.57|730d 2.90
chimera::EdgeEngine::Config s39_aptusdt_tsmom_d2_cfg{
    .symbol="aptusdt", .tag="APT-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aptusdt_tsmom_d2(s39_aptusdt_tsmom_d2_cfg);
wire_engine(s39_aptusdt_tsmom_d2);

// 53136|fetusdt,H6,ICHIMOKU,30,12,2.0|min PF=1.70|134d 2.84 n=69|180d 3.43|365d 1.70|730d 2.15
chimera::EdgeEngine::Config s39_fetusdt_ichi_h6_cfg{
    .symbol="fetusdt", .tag="FET-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_ichi_h6(s39_fetusdt_ichi_h6_cfg);
wire_engine(s39_fetusdt_ichi_h6);

// 53014|atomusdt,D2,TSMOM,18,12,2.0|min PF=4.28|134d 4.57 n=21|180d 4.66|365d 8.10|730d 4.28
chimera::EdgeEngine::Config s39_atomusdt_tsmom_d2_cfg{
    .symbol="atomusdt", .tag="ATOM-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_tsmom_d2(s39_atomusdt_tsmom_d2_cfg);
wire_engine(s39_atomusdt_tsmom_d2);

// 51903|tiausdt,D2,TSMOM,6,5,2.0|min PF=1.77|134d 6.54 n=21|180d 3.75|365d 1.77|730d 1.95
chimera::EdgeEngine::Config s39_tiausdt_tsmom_d2_cfg{
    .symbol="tiausdt", .tag="TIA-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=6, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_tsmom_d2(s39_tiausdt_tsmom_d2_cfg);
wire_engine(s39_tiausdt_tsmom_d2);

// 51149|xrpusdt,H6,ICHIMOKU,6,3,1.0|min PF=1.53|134d 1.97 n=59|180d 1.53|365d 1.63|730d 1.94
chimera::EdgeEngine::Config s39_xrpusdt_ichi_h6_cfg{
    .symbol="xrpusdt", .tag="XRP-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_xrpusdt_ichi_h6(s39_xrpusdt_ichi_h6_cfg);
wire_engine(s39_xrpusdt_ichi_h6);

// 50511|opusdt,D2,TSMOM,12,8,1.0|min PF=2.29|134d 2.33 n=23|180d 2.29|365d 2.38|730d 2.36
chimera::EdgeEngine::Config s39_opusdt_tsmom_d2_cfg{
    .symbol="opusdt", .tag="OP-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_tsmom_d2(s39_opusdt_tsmom_d2_cfg);
wire_engine(s39_opusdt_tsmom_d2);

// 50358|atomusdt,H2,ICHIMOKU,30,24,4.0|min PF=1.54|134d 2.33 n=206|180d 2.07|365d 1.54|730d 1.73
chimera::EdgeEngine::Config s39_atomusdt_ichi_h2_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_ichi_h2(s39_atomusdt_ichi_h2_cfg);
wire_engine(s39_atomusdt_ichi_h2);

// 50160|opusdt,H8,ICHIMOKU,45,5,1.0|min PF=2.06|134d 2.06 n=44|180d 2.54|365d 2.21|730d 2.59
chimera::EdgeEngine::Config s39_opusdt_ichi_h8_cfg{
    .symbol="opusdt", .tag="OP-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_ichi_h8(s39_opusdt_ichi_h8_cfg);
wire_engine(s39_opusdt_ichi_h8);

// 50135|fetusdt,D3,TSMOM,12,24,1.0|min PF=1.71|134d 5.04 n=21|180d 1.71|365d 4.03|730d 1.83
chimera::EdgeEngine::Config s39_fetusdt_tsmom_d3_cfg{
    .symbol="fetusdt", .tag="FET-TSMOM-D3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=259200, .lookback=12, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_tsmom_d3(s39_fetusdt_tsmom_d3_cfg);
wire_engine(s39_fetusdt_tsmom_d3);

// 49851|ondousdt,H6,TSMOM,60,18,3.0|min PF=2.29|134d 2.74 n=122|180d 3.11|365d 2.29|730d 2.76
chimera::EdgeEngine::Config s39_ondousdt_tsmom_h6_cfg{
    .symbol="ondousdt", .tag="ONDO-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ondousdt_tsmom_h6(s39_ondousdt_tsmom_h6_cfg);
wire_engine(s39_ondousdt_tsmom_h6);

// 49336|solusdt,H6,ICHIMOKU,60,12,2.0|min PF=2.10|134d 2.10 n=79|180d 2.10|365d 2.47|730d 2.24
chimera::EdgeEngine::Config s39_solusdt_ichi_h6_cfg{
    .symbol="solusdt", .tag="SOL-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_solusdt_ichi_h6(s39_solusdt_ichi_h6_cfg);
wire_engine(s39_solusdt_ichi_h6);

// 49070|ondousdt,H3,ICHIMOKU,18,12,2.0|min PF=2.32|134d 2.62 n=119|180d 2.32|365d 2.96|730d 2.79
chimera::EdgeEngine::Config s39_ondousdt_ichi_h3_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ondousdt_ichi_h3(s39_ondousdt_ichi_h3_cfg);
wire_engine(s39_ondousdt_ichi_h3);

// 48599|ldousdt,H8,ICHIMOKU,45,8,3.0|min PF=1.84|134d 1.84 n=26|180d 2.87|365d 2.13|730d 2.28
chimera::EdgeEngine::Config s39_ldousdt_ichi_h8_cfg{
    .symbol="ldousdt", .tag="LDO-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_ichi_h8(s39_ldousdt_ichi_h8_cfg);
wire_engine(s39_ldousdt_ichi_h8);

// 46572|atomusdt,H6,ICHIMOKU,60,12,2.0|min PF=1.52|134d 1.52 n=70|180d 2.48|365d 2.35|730d 2.76
chimera::EdgeEngine::Config s39_atomusdt_ichi_h6_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_ichi_h6(s39_atomusdt_ichi_h6_cfg);
wire_engine(s39_atomusdt_ichi_h6);

// 46111|trxusdt,H12,TSMOM,45,5,1.0|min PF=2.08|134d 2.26 n=122|180d 2.13|365d 2.08|730d 2.75
chimera::EdgeEngine::Config s39_trxusdt_tsmom_h12_cfg{
    .symbol="trxusdt", .tag="TRX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_trxusdt_tsmom_h12(s39_trxusdt_tsmom_h12_cfg);
wire_engine(s39_trxusdt_tsmom_h12);

// 45731|ondousdt,H2,ICHIMOKU,45,5,1.0|min PF=1.76|134d 1.77 n=238|180d 1.79|365d 1.76|730d 1.86
chimera::EdgeEngine::Config s39_ondousdt_ichi_h2_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ondousdt_ichi_h2(s39_ondousdt_ichi_h2_cfg);
wire_engine(s39_ondousdt_ichi_h2);

// 44797|trxusdt,H8,TSMOM,60,18,4.0|min PF=1.52|134d 1.78 n=105|180d 2.28|365d 1.52|730d 3.24
chimera::EdgeEngine::Config s39_trxusdt_tsmom_h8_cfg{
    .symbol="trxusdt", .tag="TRX-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_trxusdt_tsmom_h8(s39_trxusdt_tsmom_h8_cfg);
wire_engine(s39_trxusdt_tsmom_h8);

// 44686|injusdt,D2,TSMOM,18,3,1.0|min PF=1.63|134d 1.88 n=20|180d 1.63|365d 1.99|730d 2.47
chimera::EdgeEngine::Config s39_injusdt_tsmom_d2_cfg{
    .symbol="injusdt", .tag="INJ-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_tsmom_d2(s39_injusdt_tsmom_d2_cfg);
wire_engine(s39_injusdt_tsmom_d2);

// 44052|avaxusdt,D2,TSMOM,18,12,1.0|min PF=1.67|134d 1.67 n=23|180d 2.38|365d 3.55|730d 2.32
chimera::EdgeEngine::Config s39_avaxusdt_tsmom_d2_cfg{
    .symbol="avaxusdt", .tag="AVAX-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_avaxusdt_tsmom_d2(s39_avaxusdt_tsmom_d2_cfg);
wire_engine(s39_avaxusdt_tsmom_d2);

// 43879|filusdt,H3,ICHIMOKU,6,24,4.0|min PF=1.53|134d 1.67 n=66|180d 1.69|365d 1.74|730d 1.53
chimera::EdgeEngine::Config s39_filusdt_ichi_h3_cfg{
    .symbol="filusdt", .tag="FIL-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_ichi_h3(s39_filusdt_ichi_h3_cfg);
wire_engine(s39_filusdt_ichi_h3);

// 43824|ethusdt,H8,ICHIMOKU,12,12,1.0|min PF=1.61|134d 1.84 n=75|180d 1.61|365d 2.39|730d 2.33
chimera::EdgeEngine::Config s39_ethusdt_ichi_h8_cfg{
    .symbol="ethusdt", .tag="ETH-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=12, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=17, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ethusdt_ichi_h8(s39_ethusdt_ichi_h8_cfg);
wire_engine(s39_ethusdt_ichi_h8);

// 42924|ldousdt,H12,ICHIMOKU,60,12,2.0|min PF=2.34|134d 2.52 n=20|180d 2.54|365d 2.48|730d 2.34
chimera::EdgeEngine::Config s39_ldousdt_ichi_h12_cfg{
    .symbol="ldousdt", .tag="LDO-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_ichi_h12(s39_ldousdt_ichi_h12_cfg);
wire_engine(s39_ldousdt_ichi_h12);

// 41999|trxusdt,D1,ICHIMOKU,60,12,4.0|min PF=4.67|134d 91.68 n=35|180d 91.68|365d 4.67|730d 9.76
chimera::EdgeEngine::Config s39_trxusdt_ichi_d1_cfg{
    .symbol="trxusdt", .tag="TRX-ICHI-D1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=86400, .lookback=60, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_trxusdt_ichi_d1(s39_trxusdt_ichi_d1_cfg);
wire_engine(s39_trxusdt_ichi_d1);

// 39374|aptusdt,H6,ICHIMOKU,30,24,4.0|min PF=1.78|134d 2.20 n=50|180d 2.33|365d 2.08|730d 1.78
chimera::EdgeEngine::Config s39_aptusdt_ichi_h6_cfg{
    .symbol="aptusdt", .tag="APT-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aptusdt_ichi_h6(s39_aptusdt_ichi_h6_cfg);
wire_engine(s39_aptusdt_ichi_h6);

// 38996|trxusdt,D3,TSMOM,18,5,1.0|min PF=2.87|134d 8.92 n=21|180d 4.84|365d 2.87|730d 3.48
chimera::EdgeEngine::Config s39_trxusdt_tsmom_d3_cfg{
    .symbol="trxusdt", .tag="TRX-TSMOM-D3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=259200, .lookback=18, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_trxusdt_tsmom_d3(s39_trxusdt_tsmom_d3_cfg);
wire_engine(s39_trxusdt_tsmom_d3);

// 37435|filusdt,H12,ICHIMOKU,45,3,1.0|min PF=1.91|134d 4.89 n=22|180d 4.77|365d 1.91|730d 2.04
chimera::EdgeEngine::Config s39_filusdt_ichi_h12_cfg{
    .symbol="filusdt", .tag="FIL-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_ichi_h12(s39_filusdt_ichi_h12_cfg);
wire_engine(s39_filusdt_ichi_h12);

// 35483|atomusdt,H8,ICHIMOKU,30,24,3.0|min PF=1.56|134d 1.56 n=45|180d 1.73|365d 1.76|730d 2.45
chimera::EdgeEngine::Config s39_atomusdt_ichi_h8_cfg{
    .symbol="atomusdt", .tag="ATOM-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=30, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_ichi_h8(s39_atomusdt_ichi_h8_cfg);
wire_engine(s39_atomusdt_ichi_h8);

// 35186|tiausdt,H12,DONCHIAN,6,12,4.0|min PF=1.67|134d 4.01 n=21|180d 2.08|365d 1.67|730d 1.69
chimera::EdgeEngine::Config s39_tiausdt_dch_h12_cfg{
    .symbol="tiausdt", .tag="TIA-DCH-H12", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=43200, .lookback=6, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_dch_h12(s39_tiausdt_dch_h12_cfg);
wire_engine(s39_tiausdt_dch_h12);

// 34940|arbusdt,H6,ICHIMOKU,6,8,4.0|min PF=1.56|134d 2.08 n=69|180d 1.69|365d 1.56|730d 1.67
chimera::EdgeEngine::Config s39_arbusdt_ichi_h6_cfg{
    .symbol="arbusdt", .tag="ARB-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_arbusdt_ichi_h6(s39_arbusdt_ichi_h6_cfg);
wire_engine(s39_arbusdt_ichi_h6);

// 32484|pepeusdt,H4,BREAKOUT_PULLBACK,6,18,3.0|min PF=1.72|134d 1.72 n=27|180d 2.01|365d 2.56|730d 2.06
chimera::EdgeEngine::Config s39_pepeusdt_bopb_h4_cfg{
    .symbol="pepeusdt", .tag="PEPE-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_pepeusdt_bopb_h4(s39_pepeusdt_bopb_h4_cfg);
wire_engine(s39_pepeusdt_bopb_h4);

// 31335|fetusdt,H3,BREAKOUT_PULLBACK,12,24,4.0|min PF=1.98|134d 3.38 n=32|180d 2.45|365d 1.98|730d 3.01
chimera::EdgeEngine::Config s39_fetusdt_bopb_h3_cfg{
    .symbol="fetusdt", .tag="FET-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_bopb_h3(s39_fetusdt_bopb_h3_cfg);
wire_engine(s39_fetusdt_bopb_h3);

// 30637|tiausdt,H6,BREAKOUT_PULLBACK,6,18,4.0|min PF=2.02|134d 2.88 n=23|180d 2.02|365d 2.47|730d 2.39
chimera::EdgeEngine::Config s39_tiausdt_bopb_h6_cfg{
    .symbol="tiausdt", .tag="TIA-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_bopb_h6(s39_tiausdt_bopb_h6_cfg);
wire_engine(s39_tiausdt_bopb_h6);

// 30204|fetusdt,H2,BREAKOUT_PULLBACK,6,18,3.0|min PF=1.50|134d 2.08 n=49|180d 2.17|365d 1.74|730d 1.50
chimera::EdgeEngine::Config s39_fetusdt_bopb_h2_cfg{
    .symbol="fetusdt", .tag="FET-BOPB-H2", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=7200, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_bopb_h2(s39_fetusdt_bopb_h2_cfg);
wire_engine(s39_fetusdt_bopb_h2);

// 28756|uniusdt,H2,BOLLINGER,18,12,4.0|min PF=2.31|134d 5.60 n=25|180d 4.19|365d 3.18|730d 2.31
chimera::EdgeEngine::Config s39_uniusdt_boll_h2_cfg{
    .symbol="uniusdt", .tag="UNI-BOLL-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=18, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_boll_h2(s39_uniusdt_boll_h2_cfg);
wire_engine(s39_uniusdt_boll_h2);

// 27689|filusdt,H8,ICHIMOKU,6,12,2.0|min PF=1.73|134d 1.83 n=24|180d 1.96|365d 1.80|730d 1.73
chimera::EdgeEngine::Config s39_filusdt_ichi_h8_cfg{
    .symbol="filusdt", .tag="FIL-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_ichi_h8(s39_filusdt_ichi_h8_cfg);
wire_engine(s39_filusdt_ichi_h8);

// 27419|injusdt,H8,BREAKOUT_PULLBACK,6,18,4.0|min PF=2.00|134d 99.90 n=20|180d 233.91|365d 3.02|730d 2.00
chimera::EdgeEngine::Config s39_injusdt_bopb_h8_cfg{
    .symbol="injusdt", .tag="INJ-BOPB-H8", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=28800, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_bopb_h8(s39_injusdt_bopb_h8_cfg);
wire_engine(s39_injusdt_bopb_h8);

// 26195|dogeusdt,H8,DONCHIAN,12,18,3.0|min PF=1.70|134d 2.29 n=20|180d 3.37|365d 1.70|730d 2.67
chimera::EdgeEngine::Config s39_dogeusdt_dch_h8_cfg{
    .symbol="dogeusdt", .tag="DOGE-DCH-H8", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=28800, .lookback=12, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_dogeusdt_dch_h8(s39_dogeusdt_dch_h8_cfg);
wire_engine(s39_dogeusdt_dch_h8);

// 24730|ondousdt,H6,ICHIMOKU,30,12,3.0|min PF=2.44|134d 2.73 n=44|180d 2.73|365d 2.44|730d 4.63
chimera::EdgeEngine::Config s39_ondousdt_ichi_h6_cfg{
    .symbol="ondousdt", .tag="ONDO-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ondousdt_ichi_h6(s39_ondousdt_ichi_h6_cfg);
wire_engine(s39_ondousdt_ichi_h6);

// 24477|uniusdt,H3,KELTNER_REVERT,30,24,3.0|min PF=1.50|134d 8.18 n=21|180d 8.22|365d 3.81|730d 1.50
chimera::EdgeEngine::Config s39_uniusdt_kelt_h3_cfg{
    .symbol="uniusdt", .tag="UNI-KELT-H3", .kind=chimera::StrategyKind::KELTNER_REVERT,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_uniusdt_kelt_h3(s39_uniusdt_kelt_h3_cfg);
wire_engine(s39_uniusdt_kelt_h3);

// 24392|trxusdt,H12,ICHIMOKU,45,5,1.0|min PF=1.81|134d 2.64 n=87|180d 2.70|365d 1.99|730d 1.81
chimera::EdgeEngine::Config s39_trxusdt_ichi_h12_cfg{
    .symbol="trxusdt", .tag="TRX-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_trxusdt_ichi_h12(s39_trxusdt_ichi_h12_cfg);
wire_engine(s39_trxusdt_ichi_h12);

// 23364|injusdt,H3,BREAKOUT_PULLBACK,30,24,4.0|min PF=1.62|134d 8.24 n=24|180d 4.71|365d 2.41|730d 1.62
chimera::EdgeEngine::Config s39_injusdt_bopb_h3_cfg{
    .symbol="injusdt", .tag="INJ-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_bopb_h3(s39_injusdt_bopb_h3_cfg);
wire_engine(s39_injusdt_bopb_h3);

// 23313|suiusdt,H2,BREAKOUT_PULLBACK,12,24,4.0|min PF=1.67|134d 6.39 n=27|180d 4.91|365d 1.92|730d 1.67
chimera::EdgeEngine::Config s39_suiusdt_bopb_h2_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H2", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=7200, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_bopb_h2(s39_suiusdt_bopb_h2_cfg);
wire_engine(s39_suiusdt_bopb_h2);

// 23035|ldousdt,H3,BREAKOUT_PULLBACK,12,12,4.0|min PF=1.70|134d 3.34 n=25|180d 3.99|365d 1.70|730d 1.89
chimera::EdgeEngine::Config s39_ldousdt_bopb_h3_cfg{
    .symbol="ldousdt", .tag="LDO-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_bopb_h3(s39_ldousdt_bopb_h3_cfg);
wire_engine(s39_ldousdt_bopb_h3);

// 22859|ldousdt,H4,BREAKOUT_PULLBACK,12,18,4.0|min PF=1.83|134d 3.61 n=23|180d 2.24|365d 1.92|730d 1.83
chimera::EdgeEngine::Config s39_ldousdt_bopb_h4_cfg{
    .symbol="ldousdt", .tag="LDO-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_ldousdt_bopb_h4(s39_ldousdt_bopb_h4_cfg);
wire_engine(s39_ldousdt_bopb_h4);

// 22762|suiusdt,H6,BREAKOUT_PULLBACK,12,12,4.0|min PF=1.98|134d 3.00 n=20|180d 2.19|365d 2.33|730d 1.98
chimera::EdgeEngine::Config s39_suiusdt_bopb_h6_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_bopb_h6(s39_suiusdt_bopb_h6_cfg);
wire_engine(s39_suiusdt_bopb_h6);

// 22472|bnbusdt,H6,ICHIMOKU,30,24,4.0|min PF=1.56|134d 2.61 n=77|180d 1.65|365d 1.56|730d 1.92
chimera::EdgeEngine::Config s39_bnbusdt_ichi_h6_cfg{
    .symbol="bnbusdt", .tag="BNB-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_bnbusdt_ichi_h6(s39_bnbusdt_ichi_h6_cfg);
wire_engine(s39_bnbusdt_ichi_h6);

// 22260|nearusdt,H4,BREAKOUT_PULLBACK,6,24,4.0|min PF=1.70|134d 2.74 n=42|180d 2.14|365d 2.01|730d 1.70
chimera::EdgeEngine::Config s39_nearusdt_bopb_h4_cfg{
    .symbol="nearusdt", .tag="NEAR-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_nearusdt_bopb_h4(s39_nearusdt_bopb_h4_cfg);
wire_engine(s39_nearusdt_bopb_h4);

// 22162|suiusdt,H3,BREAKOUT_PULLBACK,6,12,4.0|min PF=1.70|134d 2.87 n=30|180d 3.67|365d 1.71|730d 1.70
chimera::EdgeEngine::Config s39_suiusdt_bopb_h3_cfg{
    .symbol="suiusdt", .tag="SUI-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=6, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_suiusdt_bopb_h3(s39_suiusdt_bopb_h3_cfg);
wire_engine(s39_suiusdt_bopb_h3);

// 22002|injusdt,H12,STOCH_RSI,45,18,3.0|min PF=1.51|134d 1.57 n=21|180d 1.67|365d 2.03|730d 1.51
chimera::EdgeEngine::Config s39_injusdt_sr_h12_cfg{
    .symbol="injusdt", .tag="INJ-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=45, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_sr_h12(s39_injusdt_sr_h12_cfg);
wire_engine(s39_injusdt_sr_h12);

// 20861|tiausdt,H8,BREAKOUT_PULLBACK,6,24,4.0|min PF=1.65|134d 2.24 n=20|180d 2.90|365d 3.41|730d 1.65
chimera::EdgeEngine::Config s39_tiausdt_bopb_h8_cfg{
    .symbol="tiausdt", .tag="TIA-BOPB-H8", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=28800, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_bopb_h8(s39_tiausdt_bopb_h8_cfg);
wire_engine(s39_tiausdt_bopb_h8);

// 19936|opusdt,H12,ICHIMOKU,18,12,3.0|min PF=1.51|134d 1.81 n=25|180d 2.45|365d 1.51|730d 1.61
chimera::EdgeEngine::Config s39_opusdt_ichi_h12_cfg{
    .symbol="opusdt", .tag="OP-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=18, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_opusdt_ichi_h12(s39_opusdt_ichi_h12_cfg);
wire_engine(s39_opusdt_ichi_h12);

// 19750|atomusdt,H12,STOCH_RSI,45,8,4.0|min PF=1.56|134d 1.81 n=22|180d 1.84|365d 3.69|730d 1.56
chimera::EdgeEngine::Config s39_atomusdt_sr_h12_cfg{
    .symbol="atomusdt", .tag="ATOM-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=45, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_sr_h12(s39_atomusdt_sr_h12_cfg);
wire_engine(s39_atomusdt_sr_h12);

// 19130|hbarusdt,H4,BREAKOUT_PULLBACK,6,5,2.0|min PF=1.55|134d 1.56 n=22|180d 1.55|365d 2.16|730d 1.97
chimera::EdgeEngine::Config s39_hbarusdt_bopb_h4_cfg{
    .symbol="hbarusdt", .tag="HBAR-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_hbarusdt_bopb_h4(s39_hbarusdt_bopb_h4_cfg);
wire_engine(s39_hbarusdt_bopb_h4);

// 18451|injusdt,H2,BOLLINGER,6,8,4.0|min PF=1.61|134d 3.58 n=21|180d 2.95|365d 2.67|730d 1.61
chimera::EdgeEngine::Config s39_injusdt_boll_h2_cfg{
    .symbol="injusdt", .tag="INJ-BOLL-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=6, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_injusdt_boll_h2(s39_injusdt_boll_h2_cfg);
wire_engine(s39_injusdt_boll_h2);

// 18090|jupusdt,H2,BREAKOUT_PULLBACK,60,12,3.0|min PF=1.65|134d 2.90 n=25|180d 2.46|365d 1.65|730d 1.69
chimera::EdgeEngine::Config s39_jupusdt_bopb_h2_cfg{
    .symbol="jupusdt", .tag="JUP-BOPB-H2", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=7200, .lookback=60, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_jupusdt_bopb_h2(s39_jupusdt_bopb_h2_cfg);
wire_engine(s39_jupusdt_bopb_h2);

// 18079|fetusdt,H3,DONCHIAN,60,12,3.0|min PF=1.54|134d 3.85 n=23|180d 1.75|365d 1.54|730d 1.76
chimera::EdgeEngine::Config s39_fetusdt_dch_h3_cfg{
    .symbol="fetusdt", .tag="FET-DCH-H3", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=10800, .lookback=60, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_fetusdt_dch_h3(s39_fetusdt_dch_h3_cfg);
wire_engine(s39_fetusdt_dch_h3);

// 17602|filusdt,H4,BREAKOUT_PULLBACK,6,5,4.0|min PF=1.52|134d 2.75 n=20|180d 3.41|365d 1.79|730d 1.52
chimera::EdgeEngine::Config s39_filusdt_bopb_h4_cfg{
    .symbol="filusdt", .tag="FIL-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_filusdt_bopb_h4(s39_filusdt_bopb_h4_cfg);
wire_engine(s39_filusdt_bopb_h4);

// 17339|tiausdt,H3,BREAKOUT_PULLBACK,18,18,4.0|min PF=1.71|134d 3.35 n=25|180d 3.52|365d 2.08|730d 1.71
chimera::EdgeEngine::Config s39_tiausdt_bopb_h3_cfg{
    .symbol="tiausdt", .tag="TIA-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=18, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_tiausdt_bopb_h3(s39_tiausdt_bopb_h3_cfg);
wire_engine(s39_tiausdt_bopb_h3);

// 17155|atomusdt,H12,WILLIAMS_R,12,8,4.0|min PF=1.54|134d 2.18 n=22|180d 2.55|365d 2.07|730d 1.54
chimera::EdgeEngine::Config s39_atomusdt_willr_h12_cfg{
    .symbol="atomusdt", .tag="ATOM-WILLR-H12", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=43200, .lookback=12, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_atomusdt_willr_h12(s39_atomusdt_willr_h12_cfg);
wire_engine(s39_atomusdt_willr_h12);

// 16231|seiusdt,H4,BREAKOUT_PULLBACK,6,18,3.0|min PF=1.67|134d 1.71 n=23|180d 1.90|365d 1.85|730d 1.67
chimera::EdgeEngine::Config s39_seiusdt_bopb_h4_cfg{
    .symbol="seiusdt", .tag="SEI-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_seiusdt_bopb_h4(s39_seiusdt_bopb_h4_cfg);
wire_engine(s39_seiusdt_bopb_h4);

// 13709|bnbusdt,H6,BREAKOUT_PULLBACK,6,24,4.0|min PF=2.76|134d 2.91 n=21|180d 3.17|365d 2.76|730d 3.11
chimera::EdgeEngine::Config s39_bnbusdt_bopb_h6_cfg{
    .symbol="bnbusdt", .tag="BNB-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_bnbusdt_bopb_h6(s39_bnbusdt_bopb_h6_cfg);
wire_engine(s39_bnbusdt_bopb_h6);

// 11664|seiusdt,H2,BOLLINGER,6,24,3.0|min PF=2.11|134d 3.78 n=20|180d 2.69|365d 2.47|730d 2.11
chimera::EdgeEngine::Config s39_seiusdt_boll_h2_cfg{
    .symbol="seiusdt", .tag="SEI-BOLL-H2", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=7200, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_seiusdt_boll_h2(s39_seiusdt_boll_h2_cfg);
wire_engine(s39_seiusdt_boll_h2);

// 11130|xrpusdt,H2,BREAKOUT_PULLBACK,60,12,3.0|min PF=1.65|134d 2.31 n=22|180d 2.31|365d 1.65|730d 2.06
chimera::EdgeEngine::Config s39_xrpusdt_bopb_h2_cfg{
    .symbol="xrpusdt", .tag="XRP-BOPB-H2", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=7200, .lookback=60, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=20, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_xrpusdt_bopb_h2(s39_xrpusdt_bopb_h2_cfg);
wire_engine(s39_xrpusdt_bopb_h2);

// 9432|aaveusdt,H3,KELTNER_REVERT,30,24,3.0|min PF=1.53|134d 1.54 n=28|180d 1.69|365d 1.53|730d 1.55
chimera::EdgeEngine::Config s39_aaveusdt_kelt_h3_cfg{
    .symbol="aaveusdt", .tag="AAVE-KELT-H3", .kind=chimera::StrategyKind::KELTNER_REVERT,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_aaveusdt_kelt_h3(s39_aaveusdt_kelt_h3_cfg);
wire_engine(s39_aaveusdt_kelt_h3);

// 7954|trxusdt,H8,BREAKOUT_PULLBACK,6,18,4.0|min PF=1.80|134d 2.03 n=31|180d 2.40|365d 1.80|730d 2.30
chimera::EdgeEngine::Config s39_trxusdt_bopb_h8_cfg{
    .symbol="trxusdt", .tag="TRX-BOPB-H8", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=28800, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_trxusdt_bopb_h8(s39_trxusdt_bopb_h8_cfg);
wire_engine(s39_trxusdt_bopb_h8);

// 7872|trxusdt,H12,BREAKOUT_PULLBACK,6,24,4.0|min PF=1.88|134d 4.82 n=27|180d 5.38|365d 1.98|730d 1.88
chimera::EdgeEngine::Config s39_trxusdt_bopb_h12_cfg{
    .symbol="trxusdt", .tag="TRX-BOPB-H12", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=43200, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s39_trxusdt_bopb_h12(s39_trxusdt_bopb_h12_cfg);
wire_engine(s39_trxusdt_bopb_h12);

// ─── PUSH_BACK BLOCKS ─────
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_h1, "jupusdt", 3600, "JUP-TSMOM-H1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_tsmom_h2, "enausdt", 7200, "ENA-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_tsmom_h2, "fetusdt", 7200, "FET-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_tsmom_h3, "injusdt", 10800, "INJ-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_WIF, &s39_wifusdt_tsmom_h3, "wifusdt", 10800, "WIF-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_h2, "jupusdt", 7200, "JUP-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_tsmom_h2, "injusdt", 7200, "INJ-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_tsmom_h3, "enausdt", 10800, "ENA-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_tsmom_h3, "fetusdt", 10800, "FET-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_tsmom_h4, "injusdt", 14400, "INJ-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_h3, "jupusdt", 10800, "JUP-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_WIF, &s39_wifusdt_tsmom_h2, "wifusdt", 7200, "WIF-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_tsmom_h2, "tiausdt", 7200, "TIA-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_tsmom_h4, "enausdt", 14400, "ENA-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_tsmom_h3, "suiusdt", 10800, "SUI-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_h2, "ldousdt", 7200, "LDO-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_tsmom_h8, "enausdt", 28800, "ENA-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_tsmom_h3, "tiausdt", 10800, "TIA-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_tsmom_h2, "suiusdt", 7200, "SUI-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_WIF, &s39_wifusdt_tsmom_h6, "wifusdt", 21600, "WIF-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_tsmom_h6, "fetusdt", 21600, "FET-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SEI, &s39_seiusdt_tsmom_h2, "seiusdt", 7200, "SEI-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_tsmom_h6, "enausdt", 21600, "ENA-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_PEPE, &s39_pepeusdt_tsmom_h3, "pepeusdt", 10800, "PEPE-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_tsmom_h6, "injusdt", 21600, "INJ-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_h4, "jupusdt", 14400, "JUP-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_tsmom_h4, "uniusdt", 14400, "UNI-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_PEPE, &s39_pepeusdt_tsmom_h6, "pepeusdt", 21600, "PEPE-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_h1, "ldousdt", 3600, "LDO-TSMOM-H1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ARB, &s39_arbusdt_tsmom_h3, "arbusdt", 10800, "ARB-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_tsmom_h6, "tiausdt", 21600, "TIA-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ARB, &s39_arbusdt_tsmom_h2, "arbusdt", 7200, "ARB-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_tsmom_h3, "aaveusdt", 10800, "AAVE-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_tsmom_h2, "aaveusdt", 7200, "AAVE-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_h4, "ldousdt", 14400, "LDO-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_h2, "opusdt", 7200, "OP-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_tsmom_h3, "maticusdt", 10800, "MATIC-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_HBAR, &s39_hbarusdt_tsmom_h2, "hbarusdt", 7200, "HBAR-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_h3, "ldousdt", 10800, "LDO-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_tsmom_h3, "uniusdt", 10800, "UNI-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_tsmom_h6, "uniusdt", 21600, "UNI-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SEI, &s39_seiusdt_tsmom_h4, "seiusdt", 14400, "SEI-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_h1, "filusdt", 3600, "FIL-TSMOM-H1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_h3, "filusdt", 10800, "FIL-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_h6, "jupusdt", 21600, "JUP-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_h4, "filusdt", 14400, "FIL-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_NEAR, &s39_nearusdt_tsmom_h2, "nearusdt", 7200, "NEAR-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_h3, "opusdt", 10800, "OP-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_ichi_h2, "enausdt", 7200, "ENA-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_h1, "opusdt", 3600, "OP-TSMOM-H1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_h12, "ldousdt", 43200, "LDO-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_tsmom_h4, "maticusdt", 14400, "MATIC-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_APT, &s39_aptusdt_tsmom_h2, "aptusdt", 7200, "APT-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_tsmom_h12, "enausdt", 43200, "ENA-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_tsmom_h12, "uniusdt", 43200, "UNI-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_h8, "jupusdt", 28800, "JUP-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AVAX, &s39_avaxusdt_tsmom_h3, "avaxusdt", 10800, "AVAX-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_DOGE, &s39_dogeusdt_tsmom_h2, "dogeusdt", 7200, "DOGE-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SEI, &s39_seiusdt_tsmom_h6, "seiusdt", 21600, "SEI-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_h8, "ldousdt", 28800, "LDO-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_tsmom_h8, "uniusdt", 28800, "UNI-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_h12, "jupusdt", 43200, "JUP-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ADA, &s39_adausdt_tsmom_h3, "adausdt", 10800, "ADA-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_HBAR, &s39_hbarusdt_tsmom_h4, "hbarusdt", 14400, "HBAR-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_ichi_h3, "suiusdt", 10800, "SUI-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_ichi_h1, "jupusdt", 3600, "JUP-ICHI-H1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_h2, "filusdt", 7200, "FIL-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_h4, "opusdt", 14400, "OP-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_tsmom_h8, "maticusdt", 28800, "MATIC-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_APT, &s39_aptusdt_tsmom_h3, "aptusdt", 10800, "APT-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_tsmom_h8, "aaveusdt", 28800, "AAVE-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_ichi_h3, "enausdt", 10800, "ENA-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_PEPE, &s39_pepeusdt_ichi_h3, "pepeusdt", 10800, "PEPE-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_tsmom_h3, "atomusdt", 10800, "ATOM-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_tsmom_h6, "maticusdt", 21600, "MATIC-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_tsmom_h2, "uniusdt", 7200, "UNI-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_ichi_h3, "injusdt", 10800, "INJ-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_ichi_h2, "tiausdt", 7200, "TIA-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_ichi_h3, "fetusdt", 10800, "FET-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_d1, "ldousdt", 86400, "LDO-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_h8, "opusdt", 28800, "OP-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_tsmom_h4, "atomusdt", 14400, "ATOM-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_h6, "ldousdt", 21600, "LDO-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_h12, "filusdt", 43200, "FIL-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LINK, &s39_linkusdt_tsmom_h2, "linkusdt", 7200, "LINK-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_ichi_h2, "jupusdt", 7200, "JUP-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_tsmom_h8, "atomusdt", 28800, "ATOM-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_tsmom_d1, "enausdt", 86400, "ENA-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_HBAR, &s39_hbarusdt_tsmom_h6, "hbarusdt", 21600, "HBAR-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_h8, "filusdt", 28800, "FIL-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_ichi_h2, "fetusdt", 7200, "FET-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ADA, &s39_adausdt_tsmom_h4, "adausdt", 14400, "ADA-TSMOM-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ADA, &s39_adausdt_tsmom_h6, "adausdt", 21600, "ADA-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_WIF, &s39_wifusdt_ichi_h3, "wifusdt", 10800, "WIF-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_tsmom_d1, "jupusdt", 86400, "JUP-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_ichi_h4, "injusdt", 14400, "INJ-ICHI-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_tsmom_d1, "uniusdt", 86400, "UNI-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_ichi_h2, "injusdt", 7200, "INJ-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ONDO, &s39_ondousdt_tsmom_h2, "ondousdt", 7200, "ONDO-TSMOM-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_h6, "filusdt", 21600, "FIL-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_h6, "opusdt", 21600, "OP-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_ichi_h3, "tiausdt", 10800, "TIA-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_ichi_h2, "suiusdt", 7200, "SUI-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ONDO, &s39_ondousdt_tsmom_h3, "ondousdt", 10800, "ONDO-TSMOM-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_ichi_h3, "aaveusdt", 10800, "AAVE-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_tsmom_h6, "atomusdt", 21600, "ATOM-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_tsmom_h12, "maticusdt", 43200, "MATIC-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_WIF, &s39_wifusdt_ichi_h2, "wifusdt", 7200, "WIF-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ENA, &s39_enausdt_ichi_h4, "enausdt", 14400, "ENA-ICHI-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_ichi_h2, "aaveusdt", 7200, "AAVE-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_XRP, &s39_xrpusdt_tsmom_h8, "xrpusdt", 28800, "XRP-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_tsmom_h12, "atomusdt", 43200, "ATOM-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_tsmom_h12, "aaveusdt", 43200, "AAVE-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_ichi_h1, "ldousdt", 3600, "LDO-ICHI-H1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_tsmom_d1, "filusdt", 86400, "FIL-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_ichi_h6, "injusdt", 21600, "INJ-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_h12, "opusdt", 43200, "OP-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ARB, &s39_arbusdt_ichi_h2, "arbusdt", 7200, "ARB-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_ichi_h2, "opusdt", 7200, "OP-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_ichi_h8, "injusdt", 28800, "INJ-ICHI-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_ichi_h3, "jupusdt", 10800, "JUP-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_ichi_h6, "tiausdt", 21600, "TIA-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_NEAR, &s39_nearusdt_ichi_h3, "nearusdt", 10800, "NEAR-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SEI, &s39_seiusdt_ichi_h2, "seiusdt", 7200, "SEI-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_ichi_h3, "atomusdt", 10800, "ATOM-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_WIF, &s39_wifusdt_ichi_h6, "wifusdt", 21600, "WIF-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ARB, &s39_arbusdt_ichi_h3, "arbusdt", 10800, "ARB-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_d1, "opusdt", 86400, "OP-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_DOGE, &s39_dogeusdt_ichi_h3, "dogeusdt", 10800, "DOGE-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_ichi_h3, "maticusdt", 10800, "MATIC-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_PEPE, &s39_pepeusdt_tsmom_d2, "pepeusdt", 172800, "PEPE-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SEI, &s39_seiusdt_ichi_h4, "seiusdt", 14400, "SEI-ICHI-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_tsmom_d1, "maticusdt", 86400, "MATIC-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_ichi_h4, "jupusdt", 14400, "JUP-ICHI-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_ichi_h3, "opusdt", 10800, "OP-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_ichi_h6, "suiusdt", 21600, "SUI-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_MATIC, &s39_maticusdt_ichi_h4, "maticusdt", 14400, "MATIC-ICHI-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_ichi_h2, "uniusdt", 7200, "UNI-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AVAX, &s39_avaxusdt_ichi_h3, "avaxusdt", 10800, "AVAX-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_PEPE, &s39_pepeusdt_ichi_h6, "pepeusdt", 21600, "PEPE-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_HBAR, &s39_hbarusdt_ichi_h6, "hbarusdt", 21600, "HBAR-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_HBAR, &s39_hbarusdt_ichi_h4, "hbarusdt", 14400, "HBAR-ICHI-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_NEAR, &s39_nearusdt_ichi_h6, "nearusdt", 21600, "NEAR-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_tsmom_d1, "aaveusdt", 86400, "AAVE-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_NEAR, &s39_nearusdt_ichi_h2, "nearusdt", 7200, "NEAR-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_ichi_h6, "jupusdt", 21600, "JUP-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_APT, &s39_aptusdt_ichi_h3, "aptusdt", 10800, "APT-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_ichi_h4, "atomusdt", 14400, "ATOM-ICHI-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_tsmom_d1, "atomusdt", 86400, "ATOM-TSMOM-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_tsmom_d2, "ldousdt", 172800, "LDO-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_ichi_h3, "ldousdt", 10800, "LDO-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_DOGE, &s39_dogeusdt_ichi_h6, "dogeusdt", 21600, "DOGE-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_APT, &s39_aptusdt_tsmom_d2, "aptusdt", 172800, "APT-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_ichi_h6, "fetusdt", 21600, "FET-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_tsmom_d2, "atomusdt", 172800, "ATOM-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_tsmom_d2, "tiausdt", 172800, "TIA-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_XRP, &s39_xrpusdt_ichi_h6, "xrpusdt", 21600, "XRP-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_tsmom_d2, "opusdt", 172800, "OP-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_ichi_h2, "atomusdt", 7200, "ATOM-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_ichi_h8, "opusdt", 28800, "OP-ICHI-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_tsmom_d3, "fetusdt", 259200, "FET-TSMOM-D3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ONDO, &s39_ondousdt_tsmom_h6, "ondousdt", 21600, "ONDO-TSMOM-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SOL, &s39_solusdt_ichi_h6, "solusdt", 21600, "SOL-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ONDO, &s39_ondousdt_ichi_h3, "ondousdt", 10800, "ONDO-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_ichi_h8, "ldousdt", 28800, "LDO-ICHI-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_ichi_h6, "atomusdt", 21600, "ATOM-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TRX, &s39_trxusdt_tsmom_h12, "trxusdt", 43200, "TRX-TSMOM-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ONDO, &s39_ondousdt_ichi_h2, "ondousdt", 7200, "ONDO-ICHI-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TRX, &s39_trxusdt_tsmom_h8, "trxusdt", 28800, "TRX-TSMOM-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_tsmom_d2, "injusdt", 172800, "INJ-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AVAX, &s39_avaxusdt_tsmom_d2, "avaxusdt", 172800, "AVAX-TSMOM-D2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_ichi_h3, "filusdt", 10800, "FIL-ICHI-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ETH, &s39_ethusdt_ichi_h8, "ethusdt", 28800, "ETH-ICHI-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_ichi_h12, "ldousdt", 43200, "LDO-ICHI-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TRX, &s39_trxusdt_ichi_d1, "trxusdt", 86400, "TRX-ICHI-D1", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_APT, &s39_aptusdt_ichi_h6, "aptusdt", 21600, "APT-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TRX, &s39_trxusdt_tsmom_d3, "trxusdt", 259200, "TRX-TSMOM-D3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_ichi_h12, "filusdt", 43200, "FIL-ICHI-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_ichi_h8, "atomusdt", 28800, "ATOM-ICHI-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_dch_h12, "tiausdt", 43200, "TIA-DCH-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ARB, &s39_arbusdt_ichi_h6, "arbusdt", 21600, "ARB-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_PEPE, &s39_pepeusdt_bopb_h4, "pepeusdt", 14400, "PEPE-BOPB-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_bopb_h3, "fetusdt", 10800, "FET-BOPB-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_bopb_h6, "tiausdt", 21600, "TIA-BOPB-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_bopb_h2, "fetusdt", 7200, "FET-BOPB-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_boll_h2, "uniusdt", 7200, "UNI-BOLL-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_ichi_h8, "filusdt", 28800, "FIL-ICHI-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_bopb_h8, "injusdt", 28800, "INJ-BOPB-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_DOGE, &s39_dogeusdt_dch_h8, "dogeusdt", 28800, "DOGE-DCH-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ONDO, &s39_ondousdt_ichi_h6, "ondousdt", 21600, "ONDO-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_UNI, &s39_uniusdt_kelt_h3, "uniusdt", 10800, "UNI-KELT-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TRX, &s39_trxusdt_ichi_h12, "trxusdt", 43200, "TRX-ICHI-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_bopb_h3, "injusdt", 10800, "INJ-BOPB-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_bopb_h2, "suiusdt", 7200, "SUI-BOPB-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_bopb_h3, "ldousdt", 10800, "LDO-BOPB-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_LDO, &s39_ldousdt_bopb_h4, "ldousdt", 14400, "LDO-BOPB-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_bopb_h6, "suiusdt", 21600, "SUI-BOPB-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_BNB, &s39_bnbusdt_ichi_h6, "bnbusdt", 21600, "BNB-ICHI-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_NEAR, &s39_nearusdt_bopb_h4, "nearusdt", 14400, "NEAR-BOPB-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SUI, &s39_suiusdt_bopb_h3, "suiusdt", 10800, "SUI-BOPB-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_sr_h12, "injusdt", 43200, "INJ-SR-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_bopb_h8, "tiausdt", 28800, "TIA-BOPB-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_OP, &s39_opusdt_ichi_h12, "opusdt", 43200, "OP-ICHI-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_sr_h12, "atomusdt", 43200, "ATOM-SR-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_HBAR, &s39_hbarusdt_bopb_h4, "hbarusdt", 14400, "HBAR-BOPB-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_INJ, &s39_injusdt_boll_h2, "injusdt", 7200, "INJ-BOLL-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_JUP, &s39_jupusdt_bopb_h2, "jupusdt", 7200, "JUP-BOPB-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FET, &s39_fetusdt_dch_h3, "fetusdt", 10800, "FET-DCH-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_FIL, &s39_filusdt_bopb_h4, "filusdt", 14400, "FIL-BOPB-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TIA, &s39_tiausdt_bopb_h3, "tiausdt", 10800, "TIA-BOPB-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_ATOM, &s39_atomusdt_willr_h12, "atomusdt", 43200, "ATOM-WILLR-H12", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SEI, &s39_seiusdt_bopb_h4, "seiusdt", 14400, "SEI-BOPB-H4", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_BNB, &s39_bnbusdt_bopb_h6, "bnbusdt", 21600, "BNB-BOPB-H6", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_SEI, &s39_seiusdt_boll_h2, "seiusdt", 7200, "SEI-BOLL-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_XRP, &s39_xrpusdt_bopb_h2, "xrpusdt", 7200, "XRP-BOPB-H2", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_AAVE, &s39_aaveusdt_kelt_h3, "aaveusdt", 10800, "AAVE-KELT-H3", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TRX, &s39_trxusdt_bopb_h8, "trxusdt", 28800, "TRX-BOPB-H8", 0.00, 0.0, 100, 730, 39});
g_slots.push_back({chimera::SYM_TRX, &s39_trxusdt_bopb_h12, "trxusdt", 43200, "TRX-BOPB-H12", 0.00, 0.0, 100, 730, 39});
