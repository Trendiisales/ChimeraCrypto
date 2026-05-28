// S40 ADDS — 187 net new (MATIC/MKR delisted dropped, RNDR remapped to RENDER)
// All survive PF>=1.5 + net_bp>0 in 4 WF windows. Tradeable on Binance Spot today.
// ─── CONFIG + ENGINE + WIRE ─────────────────────────────────────────────
// 312666|renderusdt,H1,TSMOM,60,18,3.0|min PF=1.59|134d 1.59 n=604|180d 2.19|365d 2.05|730d 2.13
chimera::EdgeEngine::Config s40_renderusdt_tsmom_h1_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_h1(s40_renderusdt_tsmom_h1_cfg);
wire_engine(s40_renderusdt_tsmom_h1);

// 301470|renderusdt,H3,TSMOM,60,3,1.0|min PF=1.97|134d 1.97 n=299|180d 2.24|365d 2.47|730d 2.23
chimera::EdgeEngine::Config s40_renderusdt_tsmom_h3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_h3(s40_renderusdt_tsmom_h3_cfg);
wire_engine(s40_renderusdt_tsmom_h3);

// 272695|renderusdt,H4,TSMOM,60,18,2.0|min PF=2.42|134d 2.42 n=167|180d 2.85|365d 2.93|730d 2.94
chimera::EdgeEngine::Config s40_renderusdt_tsmom_h4_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_h4(s40_renderusdt_tsmom_h4_cfg);
wire_engine(s40_renderusdt_tsmom_h4);

// 271541|renderusdt,H2,TSMOM,60,8,1.0|min PF=1.61|134d 1.61 n=472|180d 1.70|365d 1.80|730d 1.94
chimera::EdgeEngine::Config s40_renderusdt_tsmom_h2_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_h2(s40_renderusdt_tsmom_h2_cfg);
wire_engine(s40_renderusdt_tsmom_h2);

// 263306|renderusdt,H6,TSMOM,60,24,1.0|min PF=2.38|134d 2.38 n=136|180d 2.64|365d 2.59|730d 2.83
chimera::EdgeEngine::Config s40_renderusdt_tsmom_h6_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_h6(s40_renderusdt_tsmom_h6_cfg);
wire_engine(s40_renderusdt_tsmom_h6);

// 223247|renderusdt,H8,TSMOM,60,18,1.0|min PF=2.49|134d 2.94 n=99|180d 3.14|365d 2.92|730d 2.49
chimera::EdgeEngine::Config s40_renderusdt_tsmom_h8_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_h8(s40_renderusdt_tsmom_h8_cfg);
wire_engine(s40_renderusdt_tsmom_h8);

// 215019|renderusdt,H12,TSMOM,45,12,2.0|min PF=3.83|134d 4.71 n=44|180d 12.22|365d 6.45|730d 3.83
chimera::EdgeEngine::Config s40_renderusdt_tsmom_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=45, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_h12(s40_renderusdt_tsmom_h12_cfg);
wire_engine(s40_renderusdt_tsmom_h12);

// 207881|icpusdt,H2,TSMOM,60,5,1.0|min PF=2.04|134d 2.41 n=453|180d 2.16|365d 2.04|730d 2.05
chimera::EdgeEngine::Config s40_icpusdt_tsmom_h2_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_h2(s40_icpusdt_tsmom_h2_cfg);
wire_engine(s40_icpusdt_tsmom_h2);

// 200966|icpusdt,H1,TSMOM,60,8,1.0|min PF=1.53|134d 1.53 n=827|180d 1.63|365d 1.71|730d 1.64
chimera::EdgeEngine::Config s40_icpusdt_tsmom_h1_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_h1(s40_icpusdt_tsmom_h1_cfg);
wire_engine(s40_icpusdt_tsmom_h1);

// 193800|icpusdt,H3,TSMOM,45,8,1.0|min PF=2.11|134d 2.37 n=312|180d 2.36|365d 2.23|730d 2.11
chimera::EdgeEngine::Config s40_icpusdt_tsmom_h3_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_h3(s40_icpusdt_tsmom_h3_cfg);
wire_engine(s40_icpusdt_tsmom_h3);

// 189877|renderusdt,H1,ICHIMOKU,6,18,4.0|min PF=1.63|134d 1.63 n=321|180d 2.72|365d 2.11|730d 2.13
chimera::EdgeEngine::Config s40_renderusdt_ichi_h1_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_h1(s40_renderusdt_ichi_h1_cfg);
wire_engine(s40_renderusdt_ichi_h1);

// 182851|renderusdt,D1,TSMOM,60,3,2.0|min PF=3.31|134d 3.39 n=54|180d 4.21|365d 4.23|730d 3.31
chimera::EdgeEngine::Config s40_renderusdt_tsmom_d1_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_d1(s40_renderusdt_tsmom_d1_cfg);
wire_engine(s40_renderusdt_tsmom_d1);

// 178584|compusdt,H1,TSMOM,60,5,1.0|min PF=1.51|134d 1.68 n=793|180d 1.69|365d 1.64|730d 1.51
chimera::EdgeEngine::Config s40_compusdt_tsmom_h1_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_h1(s40_compusdt_tsmom_h1_cfg);
wire_engine(s40_compusdt_tsmom_h1);

// 176757|crvusdt,H2,TSMOM,45,12,1.0|min PF=1.64|134d 1.85 n=455|180d 1.84|365d 1.64|730d 1.84
chimera::EdgeEngine::Config s40_crvusdt_tsmom_h2_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_h2(s40_crvusdt_tsmom_h2_cfg);
wire_engine(s40_crvusdt_tsmom_h2);

// 174088|imxusdt,H2,TSMOM,45,12,2.0|min PF=1.79|134d 1.85 n=340|180d 1.79|365d 2.10|730d 2.09
chimera::EdgeEngine::Config s40_imxusdt_tsmom_h2_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_tsmom_h2(s40_imxusdt_tsmom_h2_cfg);
wire_engine(s40_imxusdt_tsmom_h2);

// 173126|compusdt,H2,TSMOM,60,24,1.0|min PF=1.80|134d 1.91 n=409|180d 1.91|365d 1.80|730d 1.95
chimera::EdgeEngine::Config s40_compusdt_tsmom_h2_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_h2(s40_compusdt_tsmom_h2_cfg);
wire_engine(s40_compusdt_tsmom_h2);

// 172712|pythusdt,H2,TSMOM,45,5,1.0|min PF=1.61|134d 1.78 n=437|180d 1.61|365d 1.77|730d 1.86
chimera::EdgeEngine::Config s40_pythusdt_tsmom_h2_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_h2(s40_pythusdt_tsmom_h2_cfg);
wire_engine(s40_pythusdt_tsmom_h2);

// 170590|compusdt,H3,TSMOM,60,5,1.0|min PF=2.07|134d 2.14 n=283|180d 2.26|365d 2.07|730d 2.21
chimera::EdgeEngine::Config s40_compusdt_tsmom_h3_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_h3(s40_compusdt_tsmom_h3_cfg);
wire_engine(s40_compusdt_tsmom_h3);

// 169621|renderusdt,D2,TSMOM,60,3,4.0|min PF=5.01|134d 5.01 n=37|180d 8.06|365d 6.81|730d 5.02
chimera::EdgeEngine::Config s40_renderusdt_tsmom_d2_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=60, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_d2(s40_renderusdt_tsmom_d2_cfg);
wire_engine(s40_renderusdt_tsmom_d2);

// 167999|icpusdt,H4,TSMOM,30,3,4.0|min PF=2.41|134d 3.56 n=205|180d 3.02|365d 2.76|730d 2.41
chimera::EdgeEngine::Config s40_icpusdt_tsmom_h4_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_h4(s40_icpusdt_tsmom_h4_cfg);
wire_engine(s40_icpusdt_tsmom_h4);

// 167541|imxusdt,H4,TSMOM,60,5,1.0|min PF=2.00|134d 2.39 n=222|180d 2.00|365d 2.62|730d 2.51
chimera::EdgeEngine::Config s40_imxusdt_tsmom_h4_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_tsmom_h4(s40_imxusdt_tsmom_h4_cfg);
wire_engine(s40_imxusdt_tsmom_h4);

// 165463|compusdt,H4,TSMOM,45,8,1.0|min PF=2.14|134d 3.15 n=216|180d 3.49|365d 2.30|730d 2.14
chimera::EdgeEngine::Config s40_compusdt_tsmom_h4_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_h4(s40_compusdt_tsmom_h4_cfg);
wire_engine(s40_compusdt_tsmom_h4);

// 150868|renderusdt,H6,ICHIMOKU,45,24,4.0|min PF=3.98|134d 4.39 n=54|180d 3.98|365d 4.17|730d 4.34
chimera::EdgeEngine::Config s40_renderusdt_ichi_h6_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_h6(s40_renderusdt_ichi_h6_cfg);
wire_engine(s40_renderusdt_ichi_h6);

// 149170|icpusdt,H2,ICHIMOKU,45,8,2.0|min PF=2.46|134d 3.22 n=178|180d 3.28|365d 3.21|730d 2.46
chimera::EdgeEngine::Config s40_icpusdt_ichi_h2_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_ichi_h2(s40_icpusdt_ichi_h2_cfg);
wire_engine(s40_icpusdt_ichi_h2);

// 148181|compusdt,H6,TSMOM,45,5,1.0|min PF=2.35|134d 3.08 n=142|180d 2.87|365d 2.36|730d 2.35
chimera::EdgeEngine::Config s40_compusdt_tsmom_h6_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_h6(s40_compusdt_tsmom_h6_cfg);
wire_engine(s40_compusdt_tsmom_h6);

// 148170|imxusdt,H6,TSMOM,30,3,1.0|min PF=2.04|134d 2.58 n=159|180d 2.04|365d 2.33|730d 2.44
chimera::EdgeEngine::Config s40_imxusdt_tsmom_h6_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_tsmom_h6(s40_imxusdt_tsmom_h6_cfg);
wire_engine(s40_imxusdt_tsmom_h6);

// 147222|manausdt,H2,TSMOM,60,24,1.0|min PF=1.73|134d 1.73 n=417|180d 1.76|365d 1.84|730d 1.86
chimera::EdgeEngine::Config s40_manausdt_tsmom_h2_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_tsmom_h2(s40_manausdt_tsmom_h2_cfg);
wire_engine(s40_manausdt_tsmom_h2);

// 146842|pythusdt,H3,TSMOM,30,5,1.0|min PF=1.64|134d 2.01 n=299|180d 1.64|365d 1.72|730d 1.94
chimera::EdgeEngine::Config s40_pythusdt_tsmom_h3_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_h3(s40_pythusdt_tsmom_h3_cfg);
wire_engine(s40_pythusdt_tsmom_h3);

// 145083|icpusdt,H8,TSMOM,45,8,1.0|min PF=2.43|134d 2.43 n=144|180d 2.51|365d 2.85|730d 2.50
chimera::EdgeEngine::Config s40_icpusdt_tsmom_h8_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_h8(s40_icpusdt_tsmom_h8_cfg);
wire_engine(s40_icpusdt_tsmom_h8);

// 144556|imxusdt,H3,TSMOM,45,5,1.0|min PF=1.67|134d 1.67 n=319|180d 1.72|365d 1.84|730d 1.90
chimera::EdgeEngine::Config s40_imxusdt_tsmom_h3_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_tsmom_h3(s40_imxusdt_tsmom_h3_cfg);
wire_engine(s40_imxusdt_tsmom_h3);

// 143132|renderusdt,D3,TSMOM,60,5,4.0|min PF=6.30|134d 6.30 n=29|180d 9.12|365d 7.26|730d 6.78
chimera::EdgeEngine::Config s40_renderusdt_tsmom_d3_cfg{
    .symbol="renderusdt", .tag="RENDER-TSMOM-D3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=259200, .lookback=60, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_tsmom_d3(s40_renderusdt_tsmom_d3_cfg);
wire_engine(s40_renderusdt_tsmom_d3);

// 142932|crvusdt,H3,TSMOM,60,3,1.0|min PF=1.50|134d 1.84 n=287|180d 1.59|365d 1.50|730d 1.98
chimera::EdgeEngine::Config s40_crvusdt_tsmom_h3_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_h3(s40_crvusdt_tsmom_h3_cfg);
wire_engine(s40_crvusdt_tsmom_h3);

// 142219|icpusdt,H6,TSMOM,45,5,1.0|min PF=2.20|134d 2.20 n=174|180d 2.37|365d 2.21|730d 2.33
chimera::EdgeEngine::Config s40_icpusdt_tsmom_h6_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_h6(s40_icpusdt_tsmom_h6_cfg);
wire_engine(s40_icpusdt_tsmom_h6);

// 140623|arkmusdt,H2,TSMOM,60,24,2.0|min PF=1.57|134d 1.93 n=279|180d 1.57|365d 1.59|730d 1.72
chimera::EdgeEngine::Config s40_arkmusdt_tsmom_h2_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_tsmom_h2(s40_arkmusdt_tsmom_h2_cfg);
wire_engine(s40_arkmusdt_tsmom_h2);

// 140372|sandusdt,H2,TSMOM,60,5,1.0|min PF=1.65|134d 1.92 n=462|180d 1.81|365d 1.65|730d 1.77
chimera::EdgeEngine::Config s40_sandusdt_tsmom_h2_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_tsmom_h2(s40_sandusdt_tsmom_h2_cfg);
wire_engine(s40_sandusdt_tsmom_h2);

// 138723|pythusdt,H4,TSMOM,30,8,1.0|min PF=1.82|134d 2.12 n=228|180d 1.92|365d 1.82|730d 2.02
chimera::EdgeEngine::Config s40_pythusdt_tsmom_h4_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_h4(s40_pythusdt_tsmom_h4_cfg);
wire_engine(s40_pythusdt_tsmom_h4);

// 138257|stxusdt,H2,TSMOM,60,8,1.0|min PF=1.57|134d 1.89 n=396|180d 1.68|365d 1.57|730d 1.70
chimera::EdgeEngine::Config s40_stxusdt_tsmom_h2_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_h2(s40_stxusdt_tsmom_h2_cfg);
wire_engine(s40_stxusdt_tsmom_h2);

// 137437|renderusdt,H8,ICHIMOKU,45,24,4.0|min PF=4.01|134d 5.65 n=34|180d 9.81|365d 5.10|730d 4.01
chimera::EdgeEngine::Config s40_renderusdt_ichi_h8_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_h8(s40_renderusdt_ichi_h8_cfg);
wire_engine(s40_renderusdt_ichi_h8);

// 135167|arkmusdt,H4,TSMOM,30,3,2.0|min PF=1.74|134d 2.73 n=185|180d 2.20|365d 1.80|730d 1.74
chimera::EdgeEngine::Config s40_arkmusdt_tsmom_h4_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_tsmom_h4(s40_arkmusdt_tsmom_h4_cfg);
wire_engine(s40_arkmusdt_tsmom_h4);

// 133590|renderusdt,H4,ICHIMOKU,30,24,4.0|min PF=1.78|134d 1.78 n=65|180d 2.19|365d 2.98|730d 3.25
chimera::EdgeEngine::Config s40_renderusdt_ichi_h4_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_h4(s40_renderusdt_ichi_h4_cfg);
wire_engine(s40_renderusdt_ichi_h4);

// 132760|renderusdt,H12,ICHIMOKU,30,18,3.0|min PF=1.75|134d 1.75 n=22|180d 7.35|365d 5.61|730d 3.71
chimera::EdgeEngine::Config s40_renderusdt_ichi_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=30, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_h12(s40_renderusdt_ichi_h12_cfg);
wire_engine(s40_renderusdt_ichi_h12);

// 132754|imxusdt,H8,TSMOM,45,3,2.0|min PF=2.61|134d 2.61 n=108|180d 2.79|365d 3.32|730d 2.68
chimera::EdgeEngine::Config s40_imxusdt_tsmom_h8_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_tsmom_h8(s40_imxusdt_tsmom_h8_cfg);
wire_engine(s40_imxusdt_tsmom_h8);

// 130725|stxusdt,H1,TSMOM,60,12,2.0|min PF=1.51|134d 1.52 n=656|180d 1.56|365d 1.51|730d 1.52
chimera::EdgeEngine::Config s40_stxusdt_tsmom_h1_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_h1(s40_stxusdt_tsmom_h1_cfg);
wire_engine(s40_stxusdt_tsmom_h1);

// 130180|compusdt,H8,TSMOM,18,3,2.0|min PF=2.43|134d 3.00 n=106|180d 2.54|365d 2.60|730d 2.43
chimera::EdgeEngine::Config s40_compusdt_tsmom_h8_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_h8(s40_compusdt_tsmom_h8_cfg);
wire_engine(s40_compusdt_tsmom_h8);

// 129621|pythusdt,H6,TSMOM,30,5,1.0|min PF=2.09|134d 2.24 n=156|180d 2.27|365d 2.18|730d 2.09
chimera::EdgeEngine::Config s40_pythusdt_tsmom_h6_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_h6(s40_pythusdt_tsmom_h6_cfg);
wire_engine(s40_pythusdt_tsmom_h6);

// 128469|icpusdt,H1,ICHIMOKU,60,24,1.0|min PF=1.52|134d 1.52 n=433|180d 1.67|365d 1.77|730d 1.68
chimera::EdgeEngine::Config s40_icpusdt_ichi_h1_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=3600, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_ichi_h1(s40_icpusdt_ichi_h1_cfg);
wire_engine(s40_icpusdt_ichi_h1);

// 128438|renderusdt,H3,ICHIMOKU,12,24,1.0|min PF=1.57|134d 1.57 n=161|180d 1.82|365d 1.79|730d 1.66
chimera::EdgeEngine::Config s40_renderusdt_ichi_h3_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=12, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_h3(s40_renderusdt_ichi_h3_cfg);
wire_engine(s40_renderusdt_ichi_h3);

// 127278|maskusdt,H2,TSMOM,60,18,1.0|min PF=1.55|134d 1.55 n=424|180d 1.61|365d 1.68|730d 1.66
chimera::EdgeEngine::Config s40_maskusdt_tsmom_h2_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_h2(s40_maskusdt_tsmom_h2_cfg);
wire_engine(s40_maskusdt_tsmom_h2);

// 126597|stxusdt,H3,TSMOM,45,12,1.0|min PF=1.70|134d 1.96 n=286|180d 1.71|365d 1.70|730d 1.71
chimera::EdgeEngine::Config s40_stxusdt_tsmom_h3_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_h3(s40_stxusdt_tsmom_h3_cfg);
wire_engine(s40_stxusdt_tsmom_h3);

// 126479|stxusdt,H6,TSMOM,30,3,1.0|min PF=2.05|134d 2.41 n=154|180d 2.30|365d 2.17|730d 2.05
chimera::EdgeEngine::Config s40_stxusdt_tsmom_h6_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_h6(s40_stxusdt_tsmom_h6_cfg);
wire_engine(s40_stxusdt_tsmom_h6);

// 125542|crvusdt,H4,TSMOM,60,24,1.0|min PF=1.64|134d 1.78 n=202|180d 1.64|365d 1.68|730d 1.97
chimera::EdgeEngine::Config s40_crvusdt_tsmom_h4_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_h4(s40_crvusdt_tsmom_h4_cfg);
wire_engine(s40_crvusdt_tsmom_h4);

// 124919|sandusdt,H3,TSMOM,30,24,1.0|min PF=1.63|134d 1.72 n=336|180d 1.80|365d 1.63|730d 1.97
chimera::EdgeEngine::Config s40_sandusdt_tsmom_h3_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_tsmom_h3(s40_sandusdt_tsmom_h3_cfg);
wire_engine(s40_sandusdt_tsmom_h3);

// 122998|maskusdt,H3,TSMOM,45,3,1.0|min PF=1.51|134d 1.62 n=308|180d 1.51|365d 1.64|730d 1.82
chimera::EdgeEngine::Config s40_maskusdt_tsmom_h3_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_h3(s40_maskusdt_tsmom_h3_cfg);
wire_engine(s40_maskusdt_tsmom_h3);

// 122689|maskusdt,H4,TSMOM,45,12,1.0|min PF=1.74|134d 2.81 n=241|180d 2.49|365d 1.74|730d 1.84
chimera::EdgeEngine::Config s40_maskusdt_tsmom_h4_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_h4(s40_maskusdt_tsmom_h4_cfg);
wire_engine(s40_maskusdt_tsmom_h4);

// 122229|arkmusdt,H6,TSMOM,60,5,1.0|min PF=1.68|134d 1.68 n=140|180d 1.90|365d 2.13|730d 2.24
chimera::EdgeEngine::Config s40_arkmusdt_tsmom_h6_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_tsmom_h6(s40_arkmusdt_tsmom_h6_cfg);
wire_engine(s40_arkmusdt_tsmom_h6);

// 120386|arkmusdt,H12,TSMOM,30,8,4.0|min PF=2.25|134d 5.73 n=57|180d 7.01|365d 5.33|730d 2.25
chimera::EdgeEngine::Config s40_arkmusdt_tsmom_h12_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_tsmom_h12(s40_arkmusdt_tsmom_h12_cfg);
wire_engine(s40_arkmusdt_tsmom_h12);

// 119831|stxusdt,H8,TSMOM,60,8,2.0|min PF=2.44|134d 6.92 n=107|180d 6.17|365d 3.26|730d 2.44
chimera::EdgeEngine::Config s40_stxusdt_tsmom_h8_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_h8(s40_stxusdt_tsmom_h8_cfg);
wire_engine(s40_stxusdt_tsmom_h8);

// 119441|dotusdt,H3,TSMOM,45,24,1.0|min PF=1.69|134d 1.69 n=285|180d 1.69|365d 1.99|730d 2.07
chimera::EdgeEngine::Config s40_dotusdt_tsmom_h3_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_dotusdt_tsmom_h3(s40_dotusdt_tsmom_h3_cfg);
wire_engine(s40_dotusdt_tsmom_h3);

// 118984|maskusdt,H1,TSMOM,45,24,4.0|min PF=1.55|134d 1.55 n=436|180d 1.55|365d 1.58|730d 1.66
chimera::EdgeEngine::Config s40_maskusdt_tsmom_h1_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=3600, .lookback=45, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_h1(s40_maskusdt_tsmom_h1_cfg);
wire_engine(s40_maskusdt_tsmom_h1);

// 118216|sandusdt,H4,TSMOM,60,8,1.0|min PF=1.66|134d 1.71 n=225|180d 1.66|365d 1.99|730d 2.12
chimera::EdgeEngine::Config s40_sandusdt_tsmom_h4_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=60, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_tsmom_h4(s40_sandusdt_tsmom_h4_cfg);
wire_engine(s40_sandusdt_tsmom_h4);

// 117498|grtusdt,H2,TSMOM,45,8,1.0|min PF=1.58|134d 1.59 n=465|180d 1.67|365d 1.58|730d 1.67
chimera::EdgeEngine::Config s40_grtusdt_tsmom_h2_cfg{
    .symbol="grtusdt", .tag="GRT-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_grtusdt_tsmom_h2(s40_grtusdt_tsmom_h2_cfg);
wire_engine(s40_grtusdt_tsmom_h2);

// 113782|pythusdt,H12,TSMOM,6,3,1.0|min PF=2.00|134d 2.85 n=92|180d 2.30|365d 2.12|730d 2.00
chimera::EdgeEngine::Config s40_pythusdt_tsmom_h12_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=6, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_h12(s40_pythusdt_tsmom_h12_cfg);
wire_engine(s40_pythusdt_tsmom_h12);

// 112169|manausdt,H6,TSMOM,30,5,1.0|min PF=1.80|134d 1.80 n=170|180d 1.97|365d 2.21|730d 2.27
chimera::EdgeEngine::Config s40_manausdt_tsmom_h6_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_tsmom_h6(s40_manausdt_tsmom_h6_cfg);
wire_engine(s40_manausdt_tsmom_h6);

// 111278|renderusdt,H2,ICHIMOKU,6,5,3.0|min PF=1.52|134d 1.59 n=179|180d 2.05|365d 1.52|730d 1.78
chimera::EdgeEngine::Config s40_renderusdt_ichi_h2_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=6, .hold_bars=5, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_h2(s40_renderusdt_ichi_h2_cfg);
wire_engine(s40_renderusdt_ichi_h2);

// 111111|compusdt,H12,TSMOM,12,24,2.0|min PF=2.10|134d 4.63 n=61|180d 3.75|365d 2.92|730d 2.10
chimera::EdgeEngine::Config s40_compusdt_tsmom_h12_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_h12(s40_compusdt_tsmom_h12_cfg);
wire_engine(s40_compusdt_tsmom_h12);

// 109896|crvusdt,H12,TSMOM,60,24,1.0|min PF=2.65|134d 3.13 n=62|180d 4.35|365d 2.65|730d 2.65
chimera::EdgeEngine::Config s40_crvusdt_tsmom_h12_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_h12(s40_crvusdt_tsmom_h12_cfg);
wire_engine(s40_crvusdt_tsmom_h12);

// 108176|icpusdt,H12,TSMOM,18,3,1.0|min PF=1.99|134d 2.79 n=85|180d 2.68|365d 2.52|730d 1.99
chimera::EdgeEngine::Config s40_icpusdt_tsmom_h12_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_h12(s40_icpusdt_tsmom_h12_cfg);
wire_engine(s40_icpusdt_tsmom_h12);

// 107918|icpusdt,D1,TSMOM,18,5,2.0|min PF=3.24|134d 4.12 n=50|180d 12.76|365d 3.83|730d 3.24
chimera::EdgeEngine::Config s40_icpusdt_tsmom_d1_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_d1(s40_icpusdt_tsmom_d1_cfg);
wire_engine(s40_icpusdt_tsmom_d1);

// 106666|pythusdt,H8,TSMOM,30,5,1.0|min PF=1.82|134d 1.82 n=121|180d 1.90|365d 2.14|730d 1.92
chimera::EdgeEngine::Config s40_pythusdt_tsmom_h8_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_h8(s40_pythusdt_tsmom_h8_cfg);
wire_engine(s40_pythusdt_tsmom_h8);

// 106604|crvusdt,H8,TSMOM,60,12,1.0|min PF=1.77|134d 1.77 n=109|180d 1.83|365d 1.82|730d 2.60
chimera::EdgeEngine::Config s40_crvusdt_tsmom_h8_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_h8(s40_crvusdt_tsmom_h8_cfg);
wire_engine(s40_crvusdt_tsmom_h8);

// 106068|arkmusdt,H8,TSMOM,60,5,1.0|min PF=1.94|134d 1.94 n=114|180d 2.02|365d 2.04|730d 2.04
chimera::EdgeEngine::Config s40_arkmusdt_tsmom_h8_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_tsmom_h8(s40_arkmusdt_tsmom_h8_cfg);
wire_engine(s40_arkmusdt_tsmom_h8);

// 105806|maskusdt,H6,TSMOM,60,3,1.0|min PF=1.75|134d 1.75 n=151|180d 1.82|365d 1.79|730d 2.31
chimera::EdgeEngine::Config s40_maskusdt_tsmom_h6_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_h6(s40_maskusdt_tsmom_h6_cfg);
wire_engine(s40_maskusdt_tsmom_h6);

// 104290|icpusdt,D2,TSMOM,18,8,2.0|min PF=4.91|134d 6.72 n=24|180d 7.19|365d 8.51|730d 4.91
chimera::EdgeEngine::Config s40_icpusdt_tsmom_d2_cfg{
    .symbol="icpusdt", .tag="ICP-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_tsmom_d2(s40_icpusdt_tsmom_d2_cfg);
wire_engine(s40_icpusdt_tsmom_d2);

// 103267|imxusdt,H12,TSMOM,60,3,2.0|min PF=2.70|134d 2.79 n=67|180d 4.31|365d 4.09|730d 2.70
chimera::EdgeEngine::Config s40_imxusdt_tsmom_h12_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_tsmom_h12(s40_imxusdt_tsmom_h12_cfg);
wire_engine(s40_imxusdt_tsmom_h12);

// 103152|stxusdt,H4,TSMOM,45,8,1.0|min PF=1.68|134d 1.90 n=221|180d 1.90|365d 1.79|730d 1.68
chimera::EdgeEngine::Config s40_stxusdt_tsmom_h4_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_h4(s40_stxusdt_tsmom_h4_cfg);
wire_engine(s40_stxusdt_tsmom_h4);

// 102167|manausdt,H4,TSMOM,45,12,2.0|min PF=2.01|134d 2.19 n=186|180d 2.23|365d 2.02|730d 2.01
chimera::EdgeEngine::Config s40_manausdt_tsmom_h4_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_tsmom_h4(s40_manausdt_tsmom_h4_cfg);
wire_engine(s40_manausdt_tsmom_h4);

// 102155|dotusdt,H4,TSMOM,30,8,1.0|min PF=1.78|134d 1.78 n=221|180d 1.82|365d 1.83|730d 1.99
chimera::EdgeEngine::Config s40_dotusdt_tsmom_h4_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_dotusdt_tsmom_h4(s40_dotusdt_tsmom_h4_cfg);
wire_engine(s40_dotusdt_tsmom_h4);

// 100620|stxusdt,H12,TSMOM,12,12,4.0|min PF=2.13|134d 7.85 n=54|180d 3.31|365d 2.96|730d 2.13
chimera::EdgeEngine::Config s40_stxusdt_tsmom_h12_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_h12(s40_stxusdt_tsmom_h12_cfg);
wire_engine(s40_stxusdt_tsmom_h12);

// 98631|maskusdt,H12,TSMOM,30,18,1.0|min PF=2.27|134d 2.34 n=72|180d 2.27|365d 2.41|730d 2.70
chimera::EdgeEngine::Config s40_maskusdt_tsmom_h12_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=30, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_h12(s40_maskusdt_tsmom_h12_cfg);
wire_engine(s40_maskusdt_tsmom_h12);

// 98159|icpusdt,H3,ICHIMOKU,18,24,2.0|min PF=2.18|134d 3.19 n=108|180d 2.93|365d 2.23|730d 2.18
chimera::EdgeEngine::Config s40_icpusdt_ichi_h3_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_ichi_h3(s40_icpusdt_ichi_h3_cfg);
wire_engine(s40_icpusdt_ichi_h3);

// 97606|sandusdt,H6,TSMOM,30,12,4.0|min PF=2.12|134d 2.16 n=114|180d 2.12|365d 2.28|730d 2.96
chimera::EdgeEngine::Config s40_sandusdt_tsmom_h6_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_tsmom_h6(s40_sandusdt_tsmom_h6_cfg);
wire_engine(s40_sandusdt_tsmom_h6);

// 97513|dotusdt,H2,TSMOM,45,5,1.0|min PF=1.50|134d 1.62 n=383|180d 1.56|365d 1.50|730d 1.55
chimera::EdgeEngine::Config s40_dotusdt_tsmom_h2_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_dotusdt_tsmom_h2(s40_dotusdt_tsmom_h2_cfg);
wire_engine(s40_dotusdt_tsmom_h2);

// 95581|dotusdt,H6,TSMOM,45,5,1.0|min PF=1.66|134d 1.83 n=131|180d 1.66|365d 2.15|730d 2.28
chimera::EdgeEngine::Config s40_dotusdt_tsmom_h6_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_dotusdt_tsmom_h6(s40_dotusdt_tsmom_h6_cfg);
wire_engine(s40_dotusdt_tsmom_h6);

// 94649|pythusdt,D1,TSMOM,18,12,1.0|min PF=2.68|134d 5.12 n=45|180d 5.07|365d 3.73|730d 2.68
chimera::EdgeEngine::Config s40_pythusdt_tsmom_d1_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=18, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_d1(s40_pythusdt_tsmom_d1_cfg);
wire_engine(s40_pythusdt_tsmom_d1);

// 92776|compusdt,H2,ICHIMOKU,12,18,2.0|min PF=1.85|134d 2.73 n=130|180d 2.43|365d 2.02|730d 1.85
chimera::EdgeEngine::Config s40_compusdt_ichi_h2_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=12, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_ichi_h2(s40_compusdt_ichi_h2_cfg);
wire_engine(s40_compusdt_ichi_h2);

// 92454|sandusdt,H8,TSMOM,45,5,1.0|min PF=2.10|134d 2.39 n=115|180d 2.56|365d 2.10|730d 2.18
chimera::EdgeEngine::Config s40_sandusdt_tsmom_h8_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_tsmom_h8(s40_sandusdt_tsmom_h8_cfg);
wire_engine(s40_sandusdt_tsmom_h8);

// 91973|maskusdt,H8,TSMOM,60,12,2.0|min PF=1.92|134d 2.41 n=99|180d 2.02|365d 1.92|730d 2.73
chimera::EdgeEngine::Config s40_maskusdt_tsmom_h8_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_h8(s40_maskusdt_tsmom_h8_cfg);
wire_engine(s40_maskusdt_tsmom_h8);

// 88180|grtusdt,H4,TSMOM,45,18,1.0|min PF=1.61|134d 1.61 n=226|180d 1.65|365d 1.68|730d 1.74
chimera::EdgeEngine::Config s40_grtusdt_tsmom_h4_cfg{
    .symbol="grtusdt", .tag="GRT-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=45, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_grtusdt_tsmom_h4(s40_grtusdt_tsmom_h4_cfg);
wire_engine(s40_grtusdt_tsmom_h4);

// 85922|imxusdt,H2,ICHIMOKU,60,12,2.0|min PF=1.50|134d 1.69 n=188|180d 1.50|365d 1.93|730d 1.78
chimera::EdgeEngine::Config s40_imxusdt_ichi_h2_cfg{
    .symbol="imxusdt", .tag="IMX-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_ichi_h2(s40_imxusdt_ichi_h2_cfg);
wire_engine(s40_imxusdt_ichi_h2);

// 85850|imxusdt,D1,TSMOM,6,3,2.0|min PF=2.25|134d 4.30 n=38|180d 2.25|365d 2.58|730d 2.44
chimera::EdgeEngine::Config s40_imxusdt_tsmom_d1_cfg{
    .symbol="imxusdt", .tag="IMX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_tsmom_d1(s40_imxusdt_tsmom_d1_cfg);
wire_engine(s40_imxusdt_tsmom_d1);

// 85398|grtusdt,H3,TSMOM,45,24,2.0|min PF=1.56|134d 1.56 n=223|180d 1.58|365d 1.71|730d 1.75
chimera::EdgeEngine::Config s40_grtusdt_tsmom_h3_cfg{
    .symbol="grtusdt", .tag="GRT-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=45, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_grtusdt_tsmom_h3(s40_grtusdt_tsmom_h3_cfg);
wire_engine(s40_grtusdt_tsmom_h3);

// 85354|arkmusdt,D1,TSMOM,45,5,2.0|min PF=2.56|134d 3.89 n=21|180d 2.98|365d 2.56|730d 2.96
chimera::EdgeEngine::Config s40_arkmusdt_tsmom_d1_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_tsmom_d1(s40_arkmusdt_tsmom_d1_cfg);
wire_engine(s40_arkmusdt_tsmom_d1);

// 83954|manausdt,H12,TSMOM,12,8,1.0|min PF=1.98|134d 2.76 n=90|180d 1.98|365d 2.05|730d 2.14
chimera::EdgeEngine::Config s40_manausdt_tsmom_h12_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_tsmom_h12(s40_manausdt_tsmom_h12_cfg);
wire_engine(s40_manausdt_tsmom_h12);

// 83844|compusdt,H6,ICHIMOKU,30,8,3.0|min PF=2.70|134d 4.68 n=44|180d 3.82|365d 2.70|730d 3.31
chimera::EdgeEngine::Config s40_compusdt_ichi_h6_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_ichi_h6(s40_compusdt_ichi_h6_cfg);
wire_engine(s40_compusdt_ichi_h6);

// 82824|manausdt,H3,TSMOM,60,12,3.0|min PF=1.52|134d 1.55 n=208|180d 1.52|365d 1.58|730d 1.85
chimera::EdgeEngine::Config s40_manausdt_tsmom_h3_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=60, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_tsmom_h3(s40_manausdt_tsmom_h3_cfg);
wire_engine(s40_manausdt_tsmom_h3);

// 82649|compusdt,D1,TSMOM,45,3,1.0|min PF=2.48|134d 2.79 n=42|180d 2.48|365d 2.68|730d 2.90
chimera::EdgeEngine::Config s40_compusdt_tsmom_d1_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_d1(s40_compusdt_tsmom_d1_cfg);
wire_engine(s40_compusdt_tsmom_d1);

// 82549|pythusdt,D2,TSMOM,12,5,2.0|min PF=4.61|134d 6.63 n=20|180d 7.36|365d 8.13|730d 4.61
chimera::EdgeEngine::Config s40_pythusdt_tsmom_d2_cfg{
    .symbol="pythusdt", .tag="PYTH-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=5, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_tsmom_d2(s40_pythusdt_tsmom_d2_cfg);
wire_engine(s40_pythusdt_tsmom_d2);

// 82260|crvusdt,H6,TSMOM,30,18,3.0|min PF=1.52|134d 1.52 n=114|180d 1.57|365d 1.66|730d 2.15
chimera::EdgeEngine::Config s40_crvusdt_tsmom_h6_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=30, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_h6(s40_crvusdt_tsmom_h6_cfg);
wire_engine(s40_crvusdt_tsmom_h6);

// 81774|renderusdt,D1,ICHIMOKU,18,12,4.0|min PF=1.77|134d 1.77 n=21|180d 3.32|365d 6.50|730d 4.44
chimera::EdgeEngine::Config s40_renderusdt_ichi_d1_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-D1", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=86400, .lookback=18, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_d1(s40_renderusdt_ichi_d1_cfg);
wire_engine(s40_renderusdt_ichi_d1);

// 81699|compusdt,H4,ICHIMOKU,18,8,4.0|min PF=2.23|134d 2.93 n=66|180d 4.14|365d 2.53|730d 2.23
chimera::EdgeEngine::Config s40_compusdt_ichi_h4_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=18, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_ichi_h4(s40_compusdt_ichi_h4_cfg);
wire_engine(s40_compusdt_ichi_h4);

// 81537|arkmusdt,H4,ICHIMOKU,45,3,2.0|min PF=1.73|134d 3.33 n=77|180d 2.98|365d 2.23|730d 1.73
chimera::EdgeEngine::Config s40_arkmusdt_ichi_h4_cfg{
    .symbol="arkmusdt", .tag="ARKM-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=45, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_ichi_h4(s40_arkmusdt_ichi_h4_cfg);
wire_engine(s40_arkmusdt_ichi_h4);

// 81065|sandusdt,H12,TSMOM,12,3,1.0|min PF=1.84|134d 2.35 n=87|180d 1.84|365d 1.94|730d 2.04
chimera::EdgeEngine::Config s40_sandusdt_tsmom_h12_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_tsmom_h12(s40_sandusdt_tsmom_h12_cfg);
wire_engine(s40_sandusdt_tsmom_h12);

// 80660|dotusdt,H8,TSMOM,60,3,1.0|min PF=1.78|134d 1.90 n=128|180d 1.78|365d 1.90|730d 2.24
chimera::EdgeEngine::Config s40_dotusdt_tsmom_h8_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_dotusdt_tsmom_h8(s40_dotusdt_tsmom_h8_cfg);
wire_engine(s40_dotusdt_tsmom_h8);

// 80092|imxusdt,H4,ICHIMOKU,12,5,1.0|min PF=1.81|134d 1.99 n=108|180d 1.81|365d 2.30|730d 2.26
chimera::EdgeEngine::Config s40_imxusdt_ichi_h4_cfg{
    .symbol="imxusdt", .tag="IMX-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=12, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_ichi_h4(s40_imxusdt_ichi_h4_cfg);
wire_engine(s40_imxusdt_ichi_h4);

// 79866|pythusdt,H2,ICHIMOKU,45,3,4.0|min PF=1.51|134d 1.58 n=208|180d 1.51|365d 1.59|730d 1.66
chimera::EdgeEngine::Config s40_pythusdt_ichi_h2_cfg{
    .symbol="pythusdt", .tag="PYTH-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_ichi_h2(s40_pythusdt_ichi_h2_cfg);
wire_engine(s40_pythusdt_ichi_h2);

// 78733|renderusdt,H8,STOCH_RSI,60,18,4.0|min PF=3.33|134d 5.69 n=32|180d 5.27|365d 3.33|730d 3.80
chimera::EdgeEngine::Config s40_renderusdt_sr_h8_cfg{
    .symbol="renderusdt", .tag="RENDER-SR-H8", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=28800, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_sr_h8(s40_renderusdt_sr_h8_cfg);
wire_engine(s40_renderusdt_sr_h8);

// 78263|dotusdt,H12,TSMOM,18,3,1.0|min PF=2.13|134d 2.16 n=78|180d 2.25|365d 2.49|730d 2.13
chimera::EdgeEngine::Config s40_dotusdt_tsmom_h12_cfg{
    .symbol="dotusdt", .tag="DOT-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_dotusdt_tsmom_h12(s40_dotusdt_tsmom_h12_cfg);
wire_engine(s40_dotusdt_tsmom_h12);

// 78137|icpusdt,H4,ICHIMOKU,12,3,3.0|min PF=2.03|134d 2.14 n=93|180d 2.03|365d 2.50|730d 2.30
chimera::EdgeEngine::Config s40_icpusdt_ichi_h4_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=12, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_ichi_h4(s40_icpusdt_ichi_h4_cfg);
wire_engine(s40_icpusdt_ichi_h4);

// 78086|manausdt,H8,TSMOM,18,24,4.0|min PF=2.00|134d 2.57 n=76|180d 2.00|365d 2.87|730d 2.24
chimera::EdgeEngine::Config s40_manausdt_tsmom_h8_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_tsmom_h8(s40_manausdt_tsmom_h8_cfg);
wire_engine(s40_manausdt_tsmom_h8);

// 77412|stxusdt,H2,ICHIMOKU,18,8,1.0|min PF=1.56|134d 1.81 n=155|180d 1.77|365d 1.61|730d 1.56
chimera::EdgeEngine::Config s40_stxusdt_ichi_h2_cfg{
    .symbol="stxusdt", .tag="STX-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=18, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_ichi_h2(s40_stxusdt_ichi_h2_cfg);
wire_engine(s40_stxusdt_ichi_h2);

// 77354|crvusdt,H3,ICHIMOKU,45,18,3.0|min PF=1.60|134d 2.48 n=101|180d 1.94|365d 1.60|730d 2.25
chimera::EdgeEngine::Config s40_crvusdt_ichi_h3_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_ichi_h3(s40_crvusdt_ichi_h3_cfg);
wire_engine(s40_crvusdt_ichi_h3);

// 76606|maskusdt,D1,TSMOM,6,24,2.0|min PF=2.34|134d 4.53 n=24|180d 2.65|365d 2.34|730d 3.56
chimera::EdgeEngine::Config s40_maskusdt_tsmom_d1_cfg{
    .symbol="maskusdt", .tag="MASK-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=24, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_tsmom_d1(s40_maskusdt_tsmom_d1_cfg);
wire_engine(s40_maskusdt_tsmom_d1);

// 76301|stxusdt,H3,ICHIMOKU,18,18,1.0|min PF=1.64|134d 2.66 n=86|180d 2.02|365d 1.67|730d 1.64
chimera::EdgeEngine::Config s40_stxusdt_ichi_h3_cfg{
    .symbol="stxusdt", .tag="STX-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_ichi_h3(s40_stxusdt_ichi_h3_cfg);
wire_engine(s40_stxusdt_ichi_h3);

// 75341|grtusdt,H8,TSMOM,60,3,1.0|min PF=1.72|134d 1.93 n=124|180d 1.89|365d 1.72|730d 2.09
chimera::EdgeEngine::Config s40_grtusdt_tsmom_h8_cfg{
    .symbol="grtusdt", .tag="GRT-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_grtusdt_tsmom_h8(s40_grtusdt_tsmom_h8_cfg);
wire_engine(s40_grtusdt_tsmom_h8);

// 75281|maskusdt,H2,ICHIMOKU,45,18,4.0|min PF=1.67|134d 2.97 n=126|180d 2.24|365d 1.67|730d 1.98
chimera::EdgeEngine::Config s40_maskusdt_ichi_h2_cfg{
    .symbol="maskusdt", .tag="MASK-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_ichi_h2(s40_maskusdt_ichi_h2_cfg);
wire_engine(s40_maskusdt_ichi_h2);

// 75210|imxusdt,H3,ICHIMOKU,45,8,2.0|min PF=1.84|134d 2.30 n=135|180d 1.84|365d 2.00|730d 1.88
chimera::EdgeEngine::Config s40_imxusdt_ichi_h3_cfg{
    .symbol="imxusdt", .tag="IMX-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_ichi_h3(s40_imxusdt_ichi_h3_cfg);
wire_engine(s40_imxusdt_ichi_h3);

// 74526|grtusdt,H12,TSMOM,18,3,1.0|min PF=1.74|134d 3.05 n=70|180d 2.24|365d 1.74|730d 2.05
chimera::EdgeEngine::Config s40_grtusdt_tsmom_h12_cfg{
    .symbol="grtusdt", .tag="GRT-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_grtusdt_tsmom_h12(s40_grtusdt_tsmom_h12_cfg);
wire_engine(s40_grtusdt_tsmom_h12);

// 73852|crvusdt,H8,ICHIMOKU,45,12,1.0|min PF=2.08|134d 2.08 n=51|180d 2.22|365d 2.30|730d 3.07
chimera::EdgeEngine::Config s40_crvusdt_ichi_h8_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_ichi_h8(s40_crvusdt_ichi_h8_cfg);
wire_engine(s40_crvusdt_ichi_h8);

// 72973|maskusdt,H3,ICHIMOKU,18,12,1.0|min PF=1.80|134d 2.65 n=91|180d 2.13|365d 1.80|730d 1.88
chimera::EdgeEngine::Config s40_maskusdt_ichi_h3_cfg{
    .symbol="maskusdt", .tag="MASK-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=12, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_maskusdt_ichi_h3(s40_maskusdt_ichi_h3_cfg);
wire_engine(s40_maskusdt_ichi_h3);

// 69986|sandusdt,H2,ICHIMOKU,45,5,1.0|min PF=1.53|134d 1.58 n=219|180d 1.53|365d 1.57|730d 1.60
chimera::EdgeEngine::Config s40_sandusdt_ichi_h2_cfg{
    .symbol="sandusdt", .tag="SAND-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_ichi_h2(s40_sandusdt_ichi_h2_cfg);
wire_engine(s40_sandusdt_ichi_h2);

// 67573|sandusdt,H3,ICHIMOKU,30,24,1.0|min PF=1.61|134d 1.61 n=140|180d 1.79|365d 1.61|730d 1.84
chimera::EdgeEngine::Config s40_sandusdt_ichi_h3_cfg{
    .symbol="sandusdt", .tag="SAND-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=30, .hold_bars=24, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_ichi_h3(s40_sandusdt_ichi_h3_cfg);
wire_engine(s40_sandusdt_ichi_h3);

// 67516|compusdt,H3,ICHIMOKU,18,8,1.0|min PF=1.51|134d 2.10 n=90|180d 2.06|365d 1.51|730d 1.68
chimera::EdgeEngine::Config s40_compusdt_ichi_h3_cfg{
    .symbol="compusdt", .tag="COMP-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=18, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_ichi_h3(s40_compusdt_ichi_h3_cfg);
wire_engine(s40_compusdt_ichi_h3);

// 67137|manausdt,D1,TSMOM,6,18,2.0|min PF=2.54|134d 4.19 n=26|180d 4.36|365d 4.41|730d 2.54
chimera::EdgeEngine::Config s40_manausdt_tsmom_d1_cfg{
    .symbol="manausdt", .tag="MANA-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_tsmom_d1(s40_manausdt_tsmom_d1_cfg);
wire_engine(s40_manausdt_tsmom_d1);

// 67041|tonusdt,H2,TSMOM,60,3,2.0|min PF=1.68|134d 2.19 n=343|180d 2.19|365d 1.68|730d 2.33
chimera::EdgeEngine::Config s40_tonusdt_tsmom_h2_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=7200, .lookback=60, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_h2(s40_tonusdt_tsmom_h2_cfg);
wire_engine(s40_tonusdt_tsmom_h2);

// 66206|manausdt,H4,ICHIMOKU,60,12,4.0|min PF=2.78|134d 2.84 n=64|180d 2.98|365d 3.19|730d 2.78
chimera::EdgeEngine::Config s40_manausdt_ichi_h4_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_ichi_h4(s40_manausdt_ichi_h4_cfg);
wire_engine(s40_manausdt_ichi_h4);

// 65130|crvusdt,H4,ICHIMOKU,30,12,3.0|min PF=1.85|134d 6.14 n=73|180d 2.31|365d 1.85|730d 1.98
chimera::EdgeEngine::Config s40_crvusdt_ichi_h4_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_ichi_h4(s40_crvusdt_ichi_h4_cfg);
wire_engine(s40_crvusdt_ichi_h4);

// 64873|crvusdt,D1,TSMOM,30,3,1.0|min PF=1.87|134d 1.88 n=32|180d 1.87|365d 2.60|730d 2.21
chimera::EdgeEngine::Config s40_crvusdt_tsmom_d1_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_d1(s40_crvusdt_tsmom_d1_cfg);
wire_engine(s40_crvusdt_tsmom_d1);

// 64564|manausdt,H6,ICHIMOKU,6,18,3.0|min PF=2.65|134d 3.89 n=53|180d 2.93|365d 3.49|730d 2.65
chimera::EdgeEngine::Config s40_manausdt_ichi_h6_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_ichi_h6(s40_manausdt_ichi_h6_cfg);
wire_engine(s40_manausdt_ichi_h6);

// 64253|arkmusdt,D2,TSMOM,12,8,2.0|min PF=1.59|134d 23.96 n=20|180d 6.41|365d 3.87|730d 1.59
chimera::EdgeEngine::Config s40_arkmusdt_tsmom_d2_cfg{
    .symbol="arkmusdt", .tag="ARKM-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=12, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_tsmom_d2(s40_arkmusdt_tsmom_d2_cfg);
wire_engine(s40_arkmusdt_tsmom_d2);

// 61741|stxusdt,D1,TSMOM,6,18,2.0|min PF=1.69|134d 12.13 n=32|180d 2.85|365d 1.94|730d 1.69
chimera::EdgeEngine::Config s40_stxusdt_tsmom_d1_cfg{
    .symbol="stxusdt", .tag="STX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=18, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_tsmom_d1(s40_stxusdt_tsmom_d1_cfg);
wire_engine(s40_stxusdt_tsmom_d1);

// 61618|renderusdt,H4,STOCH_RSI,60,24,4.0|min PF=1.88|134d 2.00 n=65|180d 1.88|365d 2.06|730d 2.35
chimera::EdgeEngine::Config s40_renderusdt_sr_h4_cfg{
    .symbol="renderusdt", .tag="RENDER-SR-H4", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_sr_h4(s40_renderusdt_sr_h4_cfg);
wire_engine(s40_renderusdt_sr_h4);

// 61478|sandusdt,H6,ICHIMOKU,18,3,3.0|min PF=1.70|134d 1.96 n=44|180d 1.70|365d 2.17|730d 2.98
chimera::EdgeEngine::Config s40_sandusdt_ichi_h6_cfg{
    .symbol="sandusdt", .tag="SAND-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=18, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_ichi_h6(s40_sandusdt_ichi_h6_cfg);
wire_engine(s40_sandusdt_ichi_h6);

// 59985|arkmusdt,H6,ICHIMOKU,30,3,2.0|min PF=1.79|134d 2.42 n=47|180d 1.99|365d 2.06|730d 1.79
chimera::EdgeEngine::Config s40_arkmusdt_ichi_h6_cfg{
    .symbol="arkmusdt", .tag="ARKM-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=30, .hold_bars=3, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_ichi_h6(s40_arkmusdt_ichi_h6_cfg);
wire_engine(s40_arkmusdt_ichi_h6);

// 58450|renderusdt,H12,STOCH_RSI,60,24,4.0|min PF=2.34|134d 2.73 n=21|180d 4.52|365d 5.02|730d 2.34
chimera::EdgeEngine::Config s40_renderusdt_sr_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_sr_h12(s40_renderusdt_sr_h12_cfg);
wire_engine(s40_renderusdt_sr_h12);

// 56847|tonusdt,H3,TSMOM,30,5,1.0|min PF=1.67|134d 1.82 n=299|180d 1.73|365d 1.67|730d 1.84
chimera::EdgeEngine::Config s40_tonusdt_tsmom_h3_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H3", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=10800, .lookback=30, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_h3(s40_tonusdt_tsmom_h3_cfg);
wire_engine(s40_tonusdt_tsmom_h3);

// 56148|crvusdt,H12,ICHIMOKU,45,3,3.0|min PF=2.16|134d 2.23 n=24|180d 2.16|365d 2.17|730d 3.50
chimera::EdgeEngine::Config s40_crvusdt_ichi_h12_cfg{
    .symbol="crvusdt", .tag="CRV-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=45, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_ichi_h12(s40_crvusdt_ichi_h12_cfg);
wire_engine(s40_crvusdt_ichi_h12);

// 54325|crvusdt,D2,TSMOM,18,18,1.0|min PF=2.01|134d 2.01 n=21|180d 2.24|365d 2.15|730d 2.92
chimera::EdgeEngine::Config s40_crvusdt_tsmom_d2_cfg{
    .symbol="crvusdt", .tag="CRV-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_crvusdt_tsmom_d2(s40_crvusdt_tsmom_d2_cfg);
wire_engine(s40_crvusdt_tsmom_d2);

// 52938|tonusdt,H8,TSMOM,60,5,1.0|min PF=2.14|134d 2.45 n=125|180d 2.32|365d 2.14|730d 2.65
chimera::EdgeEngine::Config s40_tonusdt_tsmom_h8_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=28800, .lookback=60, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_h8(s40_tonusdt_tsmom_h8_cfg);
wire_engine(s40_tonusdt_tsmom_h8);

// 52479|tonusdt,H12,TSMOM,60,8,2.0|min PF=2.69|134d 2.83 n=57|180d 2.69|365d 2.78|730d 3.19
chimera::EdgeEngine::Config s40_tonusdt_tsmom_h12_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=43200, .lookback=60, .hold_bars=8, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_h12(s40_tonusdt_tsmom_h12_cfg);
wire_engine(s40_tonusdt_tsmom_h12);

// 50375|sandusdt,D1,TSMOM,6,12,2.0|min PF=1.81|134d 2.19 n=31|180d 2.67|365d 1.81|730d 2.10
chimera::EdgeEngine::Config s40_sandusdt_tsmom_d1_cfg{
    .symbol="sandusdt", .tag="SAND-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=6, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_tsmom_d1(s40_sandusdt_tsmom_d1_cfg);
wire_engine(s40_sandusdt_tsmom_d1);

// 49505|stxusdt,H4,ICHIMOKU,30,8,1.0|min PF=1.56|134d 2.53 n=50|180d 2.15|365d 1.91|730d 1.56
chimera::EdgeEngine::Config s40_stxusdt_ichi_h4_cfg{
    .symbol="stxusdt", .tag="STX-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=30, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_stxusdt_ichi_h4(s40_stxusdt_ichi_h4_cfg);
wire_engine(s40_stxusdt_ichi_h4);

// 49282|renderusdt,D2,ICHIMOKU,12,3,1.0|min PF=1.55|134d 1.55 n=21|180d 1.77|365d 1.91|730d 2.00
chimera::EdgeEngine::Config s40_renderusdt_ichi_d2_cfg{
    .symbol="renderusdt", .tag="RENDER-ICHI-D2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=172800, .lookback=12, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_ichi_d2(s40_renderusdt_ichi_d2_cfg);
wire_engine(s40_renderusdt_ichi_d2);

// 49006|renderusdt,H4,WILLIAMS_R,60,24,4.0|min PF=1.99|134d 2.45 n=57|180d 2.09|365d 1.99|730d 2.00
chimera::EdgeEngine::Config s40_renderusdt_willr_h4_cfg{
    .symbol="renderusdt", .tag="RENDER-WILLR-H4", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_willr_h4(s40_renderusdt_willr_h4_cfg);
wire_engine(s40_renderusdt_willr_h4);

// 47194|compusdt,H12,STOCH_RSI,6,18,4.0|min PF=2.61|134d 3.37 n=20|180d 3.62|365d 2.61|730d 4.59
chimera::EdgeEngine::Config s40_compusdt_sr_h12_cfg{
    .symbol="compusdt", .tag="COMP-SR-H12", .kind=chimera::StrategyKind::STOCH_RSI,
    .tf_secs=43200, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_sr_h12(s40_compusdt_sr_h12_cfg);
wire_engine(s40_compusdt_sr_h12);

// 46974|manausdt,H12,ICHIMOKU,18,3,1.0|min PF=2.29|134d 2.72 n=28|180d 2.72|365d 2.29|730d 3.05
chimera::EdgeEngine::Config s40_manausdt_ichi_h12_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=18, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_ichi_h12(s40_manausdt_ichi_h12_cfg);
wire_engine(s40_manausdt_ichi_h12);

// 46011|tonusdt,H6,TSMOM,45,5,4.0|min PF=1.91|134d 2.36 n=104|180d 2.66|365d 1.91|730d 2.36
chimera::EdgeEngine::Config s40_tonusdt_tsmom_h6_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H6", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_h6(s40_tonusdt_tsmom_h6_cfg);
wire_engine(s40_tonusdt_tsmom_h6);

// 45911|grtusdt,H3,ICHIMOKU,6,3,4.0|min PF=1.55|134d 1.74 n=86|180d 1.87|365d 1.55|730d 1.66
chimera::EdgeEngine::Config s40_grtusdt_ichi_h3_cfg{
    .symbol="grtusdt", .tag="GRT-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=6, .hold_bars=3, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_grtusdt_ichi_h3(s40_grtusdt_ichi_h3_cfg);
wire_engine(s40_grtusdt_ichi_h3);

// 44364|renderusdt,H12,WILLIAMS_R,6,24,3.0|min PF=2.05|134d 4.01 n=23|180d 5.63|365d 5.11|730d 2.05
chimera::EdgeEngine::Config s40_renderusdt_willr_h12_cfg{
    .symbol="renderusdt", .tag="RENDER-WILLR-H12", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=43200, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_willr_h12(s40_renderusdt_willr_h12_cfg);
wire_engine(s40_renderusdt_willr_h12);

// 43571|tonusdt,H3,ICHIMOKU,45,5,4.0|min PF=1.98|134d 3.02 n=134|180d 2.15|365d 1.98|730d 3.02
chimera::EdgeEngine::Config s40_tonusdt_ichi_h3_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H3", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=10800, .lookback=45, .hold_bars=5, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_ichi_h3(s40_tonusdt_ichi_h3_cfg);
wire_engine(s40_tonusdt_ichi_h3);

// 43205|renderusdt,H3,WILLIAMS_R,6,18,4.0|min PF=1.55|134d 1.72 n=74|180d 2.38|365d 2.07|730d 1.55
chimera::EdgeEngine::Config s40_renderusdt_willr_h3_cfg{
    .symbol="renderusdt", .tag="RENDER-WILLR-H3", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=10800, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_willr_h3(s40_renderusdt_willr_h3_cfg);
wire_engine(s40_renderusdt_willr_h3);

// 42752|tonusdt,H4,TSMOM,18,12,3.0|min PF=1.59|134d 1.59 n=144|180d 1.82|365d 1.97|730d 1.80
chimera::EdgeEngine::Config s40_tonusdt_tsmom_h4_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-H4", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=14400, .lookback=18, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_h4(s40_tonusdt_tsmom_h4_cfg);
wire_engine(s40_tonusdt_tsmom_h4);

// 42060|tonusdt,H2,ICHIMOKU,60,18,4.0|min PF=2.10|134d 2.18 n=143|180d 2.55|365d 2.10|730d 2.18
chimera::EdgeEngine::Config s40_tonusdt_ichi_h2_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H2", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=7200, .lookback=60, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_ichi_h2(s40_tonusdt_ichi_h2_cfg);
wire_engine(s40_tonusdt_ichi_h2);

// 41842|pythusdt,H6,ICHIMOKU,45,5,1.0|min PF=1.62|134d 1.90 n=74|180d 1.70|365d 1.94|730d 1.62
chimera::EdgeEngine::Config s40_pythusdt_ichi_h6_cfg{
    .symbol="pythusdt", .tag="PYTH-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_ichi_h6(s40_pythusdt_ichi_h6_cfg);
wire_engine(s40_pythusdt_ichi_h6);

// 41783|renderusdt,H4,BREAKOUT_PULLBACK,18,24,4.0|min PF=2.61|134d 42.48 n=22|180d 7.64|365d 3.82|730d 2.61
chimera::EdgeEngine::Config s40_renderusdt_bopb_h4_cfg{
    .symbol="renderusdt", .tag="RENDER-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=18, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_bopb_h4(s40_renderusdt_bopb_h4_cfg);
wire_engine(s40_renderusdt_bopb_h4);

// 39900|arkmusdt,H6,WILLIAMS_R,30,24,3.0|min PF=1.85|134d 3.25 n=35|180d 3.06|365d 1.91|730d 1.85
chimera::EdgeEngine::Config s40_arkmusdt_willr_h6_cfg{
    .symbol="arkmusdt", .tag="ARKM-WILLR-H6", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=21600, .lookback=30, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_willr_h6(s40_arkmusdt_willr_h6_cfg);
wire_engine(s40_arkmusdt_willr_h6);

// 39573|dotusdt,H4,ICHIMOKU,60,24,3.0|min PF=1.56|134d 1.56 n=56|180d 1.79|365d 1.58|730d 2.25
chimera::EdgeEngine::Config s40_dotusdt_ichi_h4_cfg{
    .symbol="dotusdt", .tag="DOT-ICHI-H4", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=14400, .lookback=60, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_dotusdt_ichi_h4(s40_dotusdt_ichi_h4_cfg);
wire_engine(s40_dotusdt_ichi_h4);

// 39560|grtusdt,D1,TSMOM,12,5,3.0|min PF=1.70|134d 2.33 n=31|180d 2.00|365d 1.99|730d 1.70
chimera::EdgeEngine::Config s40_grtusdt_tsmom_d1_cfg{
    .symbol="grtusdt", .tag="GRT-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=12, .hold_bars=5, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_grtusdt_tsmom_d1(s40_grtusdt_tsmom_d1_cfg);
wire_engine(s40_grtusdt_tsmom_d1);

// 39389|icpusdt,H8,ICHIMOKU,6,18,1.0|min PF=1.74|134d 1.98 n=46|180d 1.90|365d 1.74|730d 1.91
chimera::EdgeEngine::Config s40_icpusdt_ichi_h8_cfg{
    .symbol="icpusdt", .tag="ICP-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=6, .hold_bars=18, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_icpusdt_ichi_h8(s40_icpusdt_ichi_h8_cfg);
wire_engine(s40_icpusdt_ichi_h8);

// 38360|tonusdt,H6,ICHIMOKU,60,3,3.0|min PF=2.46|134d 4.36 n=55|180d 2.86|365d 2.46|730d 4.36
chimera::EdgeEngine::Config s40_tonusdt_ichi_h6_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=60, .hold_bars=3, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_ichi_h6(s40_tonusdt_ichi_h6_cfg);
wire_engine(s40_tonusdt_ichi_h6);

// 38323|imxusdt,H8,ICHIMOKU,45,3,1.0|min PF=1.52|134d 1.96 n=40|180d 2.13|365d 1.52|730d 1.82
chimera::EdgeEngine::Config s40_imxusdt_ichi_h8_cfg{
    .symbol="imxusdt", .tag="IMX-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_ichi_h8(s40_imxusdt_ichi_h8_cfg);
wire_engine(s40_imxusdt_ichi_h8);

// 36925|imxusdt,H6,ICHIMOKU,45,18,4.0|min PF=1.66|134d 1.66 n=37|180d 2.01|365d 1.69|730d 2.01
chimera::EdgeEngine::Config s40_imxusdt_ichi_h6_cfg{
    .symbol="imxusdt", .tag="IMX-ICHI-H6", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=21600, .lookback=45, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_ichi_h6(s40_imxusdt_ichi_h6_cfg);
wire_engine(s40_imxusdt_ichi_h6);

// 35712|manausdt,H8,ICHIMOKU,45,12,3.0|min PF=1.65|134d 4.51 n=36|180d 4.72|365d 1.65|730d 1.65
chimera::EdgeEngine::Config s40_manausdt_ichi_h8_cfg{
    .symbol="manausdt", .tag="MANA-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_ichi_h8(s40_manausdt_ichi_h8_cfg);
wire_engine(s40_manausdt_ichi_h8);

// 34585|renderusdt,H3,BREAKOUT_PULLBACK,12,24,4.0|min PF=1.75|134d 31.61 n=31|180d 53.98|365d 1.94|730d 1.75
chimera::EdgeEngine::Config s40_renderusdt_bopb_h3_cfg{
    .symbol="renderusdt", .tag="RENDER-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=12, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_bopb_h3(s40_renderusdt_bopb_h3_cfg);
wire_engine(s40_renderusdt_bopb_h3);

// 33746|pythusdt,H8,ICHIMOKU,45,5,1.0|min PF=1.51|134d 1.70 n=65|180d 1.86|365d 1.71|730d 1.51
chimera::EdgeEngine::Config s40_pythusdt_ichi_h8_cfg{
    .symbol="pythusdt", .tag="PYTH-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=5, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_ichi_h8(s40_pythusdt_ichi_h8_cfg);
wire_engine(s40_pythusdt_ichi_h8);

// 33472|arkmusdt,H12,WILLIAMS_R,18,8,4.0|min PF=1.51|134d 2.95 n=25|180d 2.71|365d 2.73|730d 1.51
chimera::EdgeEngine::Config s40_arkmusdt_willr_h12_cfg{
    .symbol="arkmusdt", .tag="ARKM-WILLR-H12", .kind=chimera::StrategyKind::WILLIAMS_R,
    .tf_secs=43200, .lookback=18, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_willr_h12(s40_arkmusdt_willr_h12_cfg);
wire_engine(s40_arkmusdt_willr_h12);

// 32926|tonusdt,D1,TSMOM,30,12,2.0|min PF=2.43|134d 2.66 n=28|180d 3.31|365d 2.43|730d 3.46
chimera::EdgeEngine::Config s40_tonusdt_tsmom_d1_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=86400, .lookback=30, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_d1(s40_tonusdt_tsmom_d1_cfg);
wire_engine(s40_tonusdt_tsmom_d1);

// 30785|compusdt,D2,TSMOM,30,3,1.0|min PF=1.54|134d 1.83 n=21|180d 1.54|365d 1.57|730d 1.62
chimera::EdgeEngine::Config s40_compusdt_tsmom_d2_cfg{
    .symbol="compusdt", .tag="COMP-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=30, .hold_bars=3, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_tsmom_d2(s40_compusdt_tsmom_d2_cfg);
wire_engine(s40_compusdt_tsmom_d2);

// 29600|tonusdt,H8,ICHIMOKU,45,8,1.0|min PF=2.24|134d 3.07 n=36|180d 2.65|365d 2.24|730d 3.03
chimera::EdgeEngine::Config s40_tonusdt_ichi_h8_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=45, .hold_bars=8, .sl_atr_mult=1.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_ichi_h8(s40_tonusdt_ichi_h8_cfg);
wire_engine(s40_tonusdt_ichi_h8);

// 29470|sandusdt,H8,ICHIMOKU,60,12,2.0|min PF=1.71|134d 1.71 n=24|180d 1.71|365d 1.71|730d 1.79
chimera::EdgeEngine::Config s40_sandusdt_ichi_h8_cfg{
    .symbol="sandusdt", .tag="SAND-ICHI-H8", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=28800, .lookback=60, .hold_bars=12, .sl_atr_mult=2.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_ichi_h8(s40_sandusdt_ichi_h8_cfg);
wire_engine(s40_sandusdt_ichi_h8);

// 28599|tonusdt,H12,ICHIMOKU,6,8,3.0|min PF=2.46|134d 3.47 n=23|180d 2.46|365d 2.84|730d 3.39
chimera::EdgeEngine::Config s40_tonusdt_ichi_h12_cfg{
    .symbol="tonusdt", .tag="TON-ICHI-H12", .kind=chimera::StrategyKind::ICHIMOKU,
    .tf_secs=43200, .lookback=6, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_ichi_h12(s40_tonusdt_ichi_h12_cfg);
wire_engine(s40_tonusdt_ichi_h12);

// 28147|renderusdt,H6,BREAKOUT_PULLBACK,6,24,4.0|min PF=1.55|134d 1.55 n=22|180d 2.61|365d 1.86|730d 1.89
chimera::EdgeEngine::Config s40_renderusdt_bopb_h6_cfg{
    .symbol="renderusdt", .tag="RENDER-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_bopb_h6(s40_renderusdt_bopb_h6_cfg);
wire_engine(s40_renderusdt_bopb_h6);

// 27913|sandusdt,H12,DUAL_THRUST,45,8,4.0|min PF=1.50|134d 4.17 n=27|180d 4.43|365d 1.58|730d 1.50
chimera::EdgeEngine::Config s40_sandusdt_dt_h12_cfg{
    .symbol="sandusdt", .tag="SAND-DT-H12", .kind=chimera::StrategyKind::DUAL_THRUST,
    .tf_secs=43200, .lookback=45, .hold_bars=8, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_dt_h12(s40_sandusdt_dt_h12_cfg);
wire_engine(s40_sandusdt_dt_h12);

// 25418|pythusdt,H2,BREAKOUT_PULLBACK,12,18,4.0|min PF=1.65|134d 2.59 n=31|180d 2.63|365d 2.02|730d 1.65
chimera::EdgeEngine::Config s40_pythusdt_bopb_h2_cfg{
    .symbol="pythusdt", .tag="PYTH-BOPB-H2", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=7200, .lookback=12, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_bopb_h2(s40_pythusdt_bopb_h2_cfg);
wire_engine(s40_pythusdt_bopb_h2);

// 25229|tonusdt,D2,TSMOM,18,8,3.0|min PF=2.35|134d 3.86 n=21|180d 2.35|365d 2.69|730d 3.60
chimera::EdgeEngine::Config s40_tonusdt_tsmom_d2_cfg{
    .symbol="tonusdt", .tag="TON-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
    .tf_secs=172800, .lookback=18, .hold_bars=8, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_tsmom_d2(s40_tonusdt_tsmom_d2_cfg);
wire_engine(s40_tonusdt_tsmom_d2);

// 23700|imxusdt,H4,BREAKOUT_PULLBACK,6,18,3.0|min PF=1.52|134d 1.69 n=28|180d 1.52|365d 2.19|730d 1.75
chimera::EdgeEngine::Config s40_imxusdt_bopb_h4_cfg{
    .symbol="imxusdt", .tag="IMX-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_imxusdt_bopb_h4(s40_imxusdt_bopb_h4_cfg);
wire_engine(s40_imxusdt_bopb_h4);

// 21814|manausdt,H4,BREAKOUT_PULLBACK,6,18,3.0|min PF=1.74|134d 2.57 n=24|180d 2.94|365d 2.23|730d 1.74
chimera::EdgeEngine::Config s40_manausdt_bopb_h4_cfg{
    .symbol="manausdt", .tag="MANA-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=6, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_bopb_h4(s40_manausdt_bopb_h4_cfg);
wire_engine(s40_manausdt_bopb_h4);

// 20057|pythusdt,H4,DONCHIAN,30,24,4.0|min PF=1.60|134d 1.97 n=20|180d 1.74|365d 1.60|730d 1.61
chimera::EdgeEngine::Config s40_pythusdt_dch_h4_cfg{
    .symbol="pythusdt", .tag="PYTH-DCH-H4", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=14400, .lookback=30, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_dch_h4(s40_pythusdt_dch_h4_cfg);
wire_engine(s40_pythusdt_dch_h4);

// 19651|arkmusdt,H3,BOLLINGER,60,18,3.0|min PF=1.95|134d 3.17 n=25|180d 4.70|365d 2.29|730d 1.95
chimera::EdgeEngine::Config s40_arkmusdt_boll_h3_cfg{
    .symbol="arkmusdt", .tag="ARKM-BOLL-H3", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=10800, .lookback=60, .hold_bars=18, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_arkmusdt_boll_h3(s40_arkmusdt_boll_h3_cfg);
wire_engine(s40_arkmusdt_boll_h3);

// 19035|pythusdt,H3,BREAKOUT_PULLBACK,6,18,4.0|min PF=1.56|134d 2.03 n=36|180d 1.89|365d 1.58|730d 1.56
chimera::EdgeEngine::Config s40_pythusdt_bopb_h3_cfg{
    .symbol="pythusdt", .tag="PYTH-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=6, .hold_bars=18, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_pythusdt_bopb_h3(s40_pythusdt_bopb_h3_cfg);
wire_engine(s40_pythusdt_bopb_h3);

// 17062|manausdt,H6,BREAKOUT_PULLBACK,6,24,4.0|min PF=1.55|134d 1.59 n=20|180d 2.22|365d 2.92|730d 1.55
chimera::EdgeEngine::Config s40_manausdt_bopb_h6_cfg{
    .symbol="manausdt", .tag="MANA-BOPB-H6", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=21600, .lookback=6, .hold_bars=24, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_manausdt_bopb_h6(s40_manausdt_bopb_h6_cfg);
wire_engine(s40_manausdt_bopb_h6);

// 16368|sandusdt,H6,DONCHIAN,18,12,4.0|min PF=1.50|134d 1.85 n=24|180d 1.50|365d 1.83|730d 1.75
chimera::EdgeEngine::Config s40_sandusdt_dch_h6_cfg{
    .symbol="sandusdt", .tag="SAND-DCH-H6", .kind=chimera::StrategyKind::DONCHIAN,
    .tf_secs=21600, .lookback=18, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_dch_h6(s40_sandusdt_dch_h6_cfg);
wire_engine(s40_sandusdt_dch_h6);

// 15595|renderusdt,H3,BOLLINGER,6,24,3.0|min PF=1.59|134d 119.33 n=20|180d 163.76|365d 2.93|730d 1.59
chimera::EdgeEngine::Config s40_renderusdt_boll_h3_cfg{
    .symbol="renderusdt", .tag="RENDER-BOLL-H3", .kind=chimera::StrategyKind::BOLLINGER,
    .tf_secs=10800, .lookback=6, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_renderusdt_boll_h3(s40_renderusdt_boll_h3_cfg);
wire_engine(s40_renderusdt_boll_h3);

// 15178|sandusdt,H3,BREAKOUT_PULLBACK,12,12,4.0|min PF=1.56|134d 3.59 n=20|180d 2.77|365d 1.56|730d 1.69
chimera::EdgeEngine::Config s40_sandusdt_bopb_h3_cfg{
    .symbol="sandusdt", .tag="SAND-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_sandusdt_bopb_h3(s40_sandusdt_bopb_h3_cfg);
wire_engine(s40_sandusdt_bopb_h3);

// 14548|tonusdt,H4,BREAKOUT_PULLBACK,12,12,4.0|min PF=2.52|134d 5.45 n=20|180d 7.74|365d 2.52|730d 5.45
chimera::EdgeEngine::Config s40_tonusdt_bopb_h4_cfg{
    .symbol="tonusdt", .tag="TON-BOPB-H4", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=14400, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_bopb_h4(s40_tonusdt_bopb_h4_cfg);
wire_engine(s40_tonusdt_bopb_h4);

// 14287|tonusdt,H3,BREAKOUT_PULLBACK,12,12,4.0|min PF=1.64|134d 3.25 n=30|180d 3.63|365d 1.64|730d 3.25
chimera::EdgeEngine::Config s40_tonusdt_bopb_h3_cfg{
    .symbol="tonusdt", .tag="TON-BOPB-H3", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=10800, .lookback=12, .hold_bars=12, .sl_atr_mult=4.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_bopb_h3(s40_tonusdt_bopb_h3_cfg);
wire_engine(s40_tonusdt_bopb_h3);

// 11947|tonusdt,H2,BREAKOUT_PULLBACK,60,24,3.0|min PF=1.62|134d 4.36 n=22|180d 4.46|365d 1.62|730d 4.36
chimera::EdgeEngine::Config s40_tonusdt_bopb_h2_cfg{
    .symbol="tonusdt", .tag="TON-BOPB-H2", .kind=chimera::StrategyKind::BREAKOUT_PULLBACK,
    .tf_secs=7200, .lookback=60, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_tonusdt_bopb_h2(s40_tonusdt_bopb_h2_cfg);
wire_engine(s40_tonusdt_bopb_h2);

// 10729|compusdt,H4,KELTNER_REVERT,18,24,3.0|min PF=1.50|134d 10.62 n=21|180d 2.77|365d 2.86|730d 1.50
chimera::EdgeEngine::Config s40_compusdt_kelt_h4_cfg{
    .symbol="compusdt", .tag="COMP-KELT-H4", .kind=chimera::StrategyKind::KELTNER_REVERT,
    .tf_secs=14400, .lookback=18, .hold_bars=24, .sl_atr_mult=3.0, .atr_period=14,
    .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=128,
    .trail_arm_atr=1.0, .trail_dist_atr=0.5, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
};
chimera::EdgeEngine s40_compusdt_kelt_h4(s40_compusdt_kelt_h4_cfg);
wire_engine(s40_compusdt_kelt_h4);

// ─── PUSH_BACK BLOCKS ─────
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_h1, "renderusdt", 3600, "RENDER-TSMOM-H1", 1.59, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_h3, "renderusdt", 10800, "RENDER-TSMOM-H3", 1.97, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_h4, "renderusdt", 14400, "RENDER-TSMOM-H4", 2.42, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_h2, "renderusdt", 7200, "RENDER-TSMOM-H2", 1.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_h6, "renderusdt", 21600, "RENDER-TSMOM-H6", 2.38, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_h8, "renderusdt", 28800, "RENDER-TSMOM-H8", 2.49, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_h12, "renderusdt", 43200, "RENDER-TSMOM-H12", 3.83, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_h2, "icpusdt", 7200, "ICP-TSMOM-H2", 2.04, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_h1, "icpusdt", 3600, "ICP-TSMOM-H1", 1.53, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_h3, "icpusdt", 10800, "ICP-TSMOM-H3", 2.11, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_h1, "renderusdt", 3600, "RENDER-ICHI-H1", 1.63, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_d1, "renderusdt", 86400, "RENDER-TSMOM-D1", 3.31, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_h1, "compusdt", 3600, "COMP-TSMOM-H1", 1.51, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_h2, "crvusdt", 7200, "CRV-TSMOM-H2", 1.64, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_tsmom_h2, "imxusdt", 7200, "IMX-TSMOM-H2", 1.79, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_h2, "compusdt", 7200, "COMP-TSMOM-H2", 1.80, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_h2, "pythusdt", 7200, "PYTH-TSMOM-H2", 1.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_h3, "compusdt", 10800, "COMP-TSMOM-H3", 2.07, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_d2, "renderusdt", 172800, "RENDER-TSMOM-D2", 5.01, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_h4, "icpusdt", 14400, "ICP-TSMOM-H4", 2.41, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_tsmom_h4, "imxusdt", 14400, "IMX-TSMOM-H4", 2.00, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_h4, "compusdt", 14400, "COMP-TSMOM-H4", 2.14, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_h6, "renderusdt", 21600, "RENDER-ICHI-H6", 3.98, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_ichi_h2, "icpusdt", 7200, "ICP-ICHI-H2", 2.46, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_h6, "compusdt", 21600, "COMP-TSMOM-H6", 2.35, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_tsmom_h6, "imxusdt", 21600, "IMX-TSMOM-H6", 2.04, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_tsmom_h2, "manausdt", 7200, "MANA-TSMOM-H2", 1.73, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_h3, "pythusdt", 10800, "PYTH-TSMOM-H3", 1.64, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_h8, "icpusdt", 28800, "ICP-TSMOM-H8", 2.43, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_tsmom_h3, "imxusdt", 10800, "IMX-TSMOM-H3", 1.67, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_tsmom_d3, "renderusdt", 259200, "RENDER-TSMOM-D3", 6.30, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_h3, "crvusdt", 10800, "CRV-TSMOM-H3", 1.50, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_h6, "icpusdt", 21600, "ICP-TSMOM-H6", 2.20, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_tsmom_h2, "arkmusdt", 7200, "ARKM-TSMOM-H2", 1.57, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_tsmom_h2, "sandusdt", 7200, "SAND-TSMOM-H2", 1.65, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_h4, "pythusdt", 14400, "PYTH-TSMOM-H4", 1.82, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_h2, "stxusdt", 7200, "STX-TSMOM-H2", 1.57, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_h8, "renderusdt", 28800, "RENDER-ICHI-H8", 4.01, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_tsmom_h4, "arkmusdt", 14400, "ARKM-TSMOM-H4", 1.74, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_h4, "renderusdt", 14400, "RENDER-ICHI-H4", 1.78, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_h12, "renderusdt", 43200, "RENDER-ICHI-H12", 1.75, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_tsmom_h8, "imxusdt", 28800, "IMX-TSMOM-H8", 2.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_h1, "stxusdt", 3600, "STX-TSMOM-H1", 1.51, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_h8, "compusdt", 28800, "COMP-TSMOM-H8", 2.43, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_h6, "pythusdt", 21600, "PYTH-TSMOM-H6", 2.09, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_ichi_h1, "icpusdt", 3600, "ICP-ICHI-H1", 1.52, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_h3, "renderusdt", 10800, "RENDER-ICHI-H3", 1.57, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_h2, "maskusdt", 7200, "MASK-TSMOM-H2", 1.55, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_h3, "stxusdt", 10800, "STX-TSMOM-H3", 1.70, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_h6, "stxusdt", 21600, "STX-TSMOM-H6", 2.05, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_h4, "crvusdt", 14400, "CRV-TSMOM-H4", 1.64, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_tsmom_h3, "sandusdt", 10800, "SAND-TSMOM-H3", 1.63, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_h3, "maskusdt", 10800, "MASK-TSMOM-H3", 1.51, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_h4, "maskusdt", 14400, "MASK-TSMOM-H4", 1.74, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_tsmom_h6, "arkmusdt", 21600, "ARKM-TSMOM-H6", 1.68, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_tsmom_h12, "arkmusdt", 43200, "ARKM-TSMOM-H12", 2.25, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_h8, "stxusdt", 28800, "STX-TSMOM-H8", 2.44, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_DOT, &s40_dotusdt_tsmom_h3, "dotusdt", 10800, "DOT-TSMOM-H3", 1.69, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_h1, "maskusdt", 3600, "MASK-TSMOM-H1", 1.55, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_tsmom_h4, "sandusdt", 14400, "SAND-TSMOM-H4", 1.66, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_GRT, &s40_grtusdt_tsmom_h2, "grtusdt", 7200, "GRT-TSMOM-H2", 1.58, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_h12, "pythusdt", 43200, "PYTH-TSMOM-H12", 2.00, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_tsmom_h6, "manausdt", 21600, "MANA-TSMOM-H6", 1.80, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_h2, "renderusdt", 7200, "RENDER-ICHI-H2", 1.52, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_h12, "compusdt", 43200, "COMP-TSMOM-H12", 2.10, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_h12, "crvusdt", 43200, "CRV-TSMOM-H12", 2.65, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_h12, "icpusdt", 43200, "ICP-TSMOM-H12", 1.99, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_d1, "icpusdt", 86400, "ICP-TSMOM-D1", 3.24, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_h8, "pythusdt", 28800, "PYTH-TSMOM-H8", 1.82, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_h8, "crvusdt", 28800, "CRV-TSMOM-H8", 1.77, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_tsmom_h8, "arkmusdt", 28800, "ARKM-TSMOM-H8", 1.94, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_h6, "maskusdt", 21600, "MASK-TSMOM-H6", 1.75, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_tsmom_d2, "icpusdt", 172800, "ICP-TSMOM-D2", 4.91, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_tsmom_h12, "imxusdt", 43200, "IMX-TSMOM-H12", 2.70, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_h4, "stxusdt", 14400, "STX-TSMOM-H4", 1.68, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_tsmom_h4, "manausdt", 14400, "MANA-TSMOM-H4", 2.01, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_DOT, &s40_dotusdt_tsmom_h4, "dotusdt", 14400, "DOT-TSMOM-H4", 1.78, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_h12, "stxusdt", 43200, "STX-TSMOM-H12", 2.13, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_h12, "maskusdt", 43200, "MASK-TSMOM-H12", 2.27, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_ichi_h3, "icpusdt", 10800, "ICP-ICHI-H3", 2.18, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_tsmom_h6, "sandusdt", 21600, "SAND-TSMOM-H6", 2.12, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_DOT, &s40_dotusdt_tsmom_h2, "dotusdt", 7200, "DOT-TSMOM-H2", 1.50, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_DOT, &s40_dotusdt_tsmom_h6, "dotusdt", 21600, "DOT-TSMOM-H6", 1.66, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_d1, "pythusdt", 86400, "PYTH-TSMOM-D1", 2.68, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_ichi_h2, "compusdt", 7200, "COMP-ICHI-H2", 1.85, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_tsmom_h8, "sandusdt", 28800, "SAND-TSMOM-H8", 2.10, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_h8, "maskusdt", 28800, "MASK-TSMOM-H8", 1.92, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_GRT, &s40_grtusdt_tsmom_h4, "grtusdt", 14400, "GRT-TSMOM-H4", 1.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_ichi_h2, "imxusdt", 7200, "IMX-ICHI-H2", 1.50, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_tsmom_d1, "imxusdt", 86400, "IMX-TSMOM-D1", 2.25, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_GRT, &s40_grtusdt_tsmom_h3, "grtusdt", 10800, "GRT-TSMOM-H3", 1.56, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_tsmom_d1, "arkmusdt", 86400, "ARKM-TSMOM-D1", 2.56, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_tsmom_h12, "manausdt", 43200, "MANA-TSMOM-H12", 1.98, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_ichi_h6, "compusdt", 21600, "COMP-ICHI-H6", 2.70, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_tsmom_h3, "manausdt", 10800, "MANA-TSMOM-H3", 1.52, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_d1, "compusdt", 86400, "COMP-TSMOM-D1", 2.48, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_tsmom_d2, "pythusdt", 172800, "PYTH-TSMOM-D2", 4.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_h6, "crvusdt", 21600, "CRV-TSMOM-H6", 1.52, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_d1, "renderusdt", 86400, "RENDER-ICHI-D1", 1.77, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_ichi_h4, "compusdt", 14400, "COMP-ICHI-H4", 2.23, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_ichi_h4, "arkmusdt", 14400, "ARKM-ICHI-H4", 1.73, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_tsmom_h12, "sandusdt", 43200, "SAND-TSMOM-H12", 1.84, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_DOT, &s40_dotusdt_tsmom_h8, "dotusdt", 28800, "DOT-TSMOM-H8", 1.78, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_ichi_h4, "imxusdt", 14400, "IMX-ICHI-H4", 1.81, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_ichi_h2, "pythusdt", 7200, "PYTH-ICHI-H2", 1.51, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_sr_h8, "renderusdt", 28800, "RENDER-SR-H8", 3.33, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_DOT, &s40_dotusdt_tsmom_h12, "dotusdt", 43200, "DOT-TSMOM-H12", 2.13, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_ichi_h4, "icpusdt", 14400, "ICP-ICHI-H4", 2.03, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_tsmom_h8, "manausdt", 28800, "MANA-TSMOM-H8", 2.00, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_ichi_h2, "stxusdt", 7200, "STX-ICHI-H2", 1.56, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_ichi_h3, "crvusdt", 10800, "CRV-ICHI-H3", 1.60, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_tsmom_d1, "maskusdt", 86400, "MASK-TSMOM-D1", 2.34, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_ichi_h3, "stxusdt", 10800, "STX-ICHI-H3", 1.64, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_GRT, &s40_grtusdt_tsmom_h8, "grtusdt", 28800, "GRT-TSMOM-H8", 1.72, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_ichi_h2, "maskusdt", 7200, "MASK-ICHI-H2", 1.67, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_ichi_h3, "imxusdt", 10800, "IMX-ICHI-H3", 1.84, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_GRT, &s40_grtusdt_tsmom_h12, "grtusdt", 43200, "GRT-TSMOM-H12", 1.74, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_ichi_h8, "crvusdt", 28800, "CRV-ICHI-H8", 2.08, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MASK, &s40_maskusdt_ichi_h3, "maskusdt", 10800, "MASK-ICHI-H3", 1.80, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_ichi_h2, "sandusdt", 7200, "SAND-ICHI-H2", 1.53, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_ichi_h3, "sandusdt", 10800, "SAND-ICHI-H3", 1.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_ichi_h3, "compusdt", 10800, "COMP-ICHI-H3", 1.51, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_tsmom_d1, "manausdt", 86400, "MANA-TSMOM-D1", 2.54, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_h2, "tonusdt", 7200, "TON-TSMOM-H2", 1.68, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_ichi_h4, "manausdt", 14400, "MANA-ICHI-H4", 2.78, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_ichi_h4, "crvusdt", 14400, "CRV-ICHI-H4", 1.85, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_d1, "crvusdt", 86400, "CRV-TSMOM-D1", 1.87, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_ichi_h6, "manausdt", 21600, "MANA-ICHI-H6", 2.65, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_tsmom_d2, "arkmusdt", 172800, "ARKM-TSMOM-D2", 1.59, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_tsmom_d1, "stxusdt", 86400, "STX-TSMOM-D1", 1.69, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_sr_h4, "renderusdt", 14400, "RENDER-SR-H4", 1.88, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_ichi_h6, "sandusdt", 21600, "SAND-ICHI-H6", 1.70, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_ichi_h6, "arkmusdt", 21600, "ARKM-ICHI-H6", 1.79, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_sr_h12, "renderusdt", 43200, "RENDER-SR-H12", 2.34, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_h3, "tonusdt", 10800, "TON-TSMOM-H3", 1.67, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_ichi_h12, "crvusdt", 43200, "CRV-ICHI-H12", 2.16, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_CRV, &s40_crvusdt_tsmom_d2, "crvusdt", 172800, "CRV-TSMOM-D2", 2.01, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_h8, "tonusdt", 28800, "TON-TSMOM-H8", 2.14, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_h12, "tonusdt", 43200, "TON-TSMOM-H12", 2.69, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_tsmom_d1, "sandusdt", 86400, "SAND-TSMOM-D1", 1.81, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_STX, &s40_stxusdt_ichi_h4, "stxusdt", 14400, "STX-ICHI-H4", 1.56, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_ichi_d2, "renderusdt", 172800, "RENDER-ICHI-D2", 1.55, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_willr_h4, "renderusdt", 14400, "RENDER-WILLR-H4", 1.99, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_sr_h12, "compusdt", 43200, "COMP-SR-H12", 2.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_ichi_h12, "manausdt", 43200, "MANA-ICHI-H12", 2.29, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_h6, "tonusdt", 21600, "TON-TSMOM-H6", 1.91, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_GRT, &s40_grtusdt_ichi_h3, "grtusdt", 10800, "GRT-ICHI-H3", 1.55, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_willr_h12, "renderusdt", 43200, "RENDER-WILLR-H12", 2.05, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_ichi_h3, "tonusdt", 10800, "TON-ICHI-H3", 1.98, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_willr_h3, "renderusdt", 10800, "RENDER-WILLR-H3", 1.55, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_h4, "tonusdt", 14400, "TON-TSMOM-H4", 1.59, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_ichi_h2, "tonusdt", 7200, "TON-ICHI-H2", 2.10, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_ichi_h6, "pythusdt", 21600, "PYTH-ICHI-H6", 1.62, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_bopb_h4, "renderusdt", 14400, "RENDER-BOPB-H4", 2.61, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_willr_h6, "arkmusdt", 21600, "ARKM-WILLR-H6", 1.85, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_DOT, &s40_dotusdt_ichi_h4, "dotusdt", 14400, "DOT-ICHI-H4", 1.56, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_GRT, &s40_grtusdt_tsmom_d1, "grtusdt", 86400, "GRT-TSMOM-D1", 1.70, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ICP, &s40_icpusdt_ichi_h8, "icpusdt", 28800, "ICP-ICHI-H8", 1.74, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_ichi_h6, "tonusdt", 21600, "TON-ICHI-H6", 2.46, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_ichi_h8, "imxusdt", 28800, "IMX-ICHI-H8", 1.52, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_ichi_h6, "imxusdt", 21600, "IMX-ICHI-H6", 1.66, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_ichi_h8, "manausdt", 28800, "MANA-ICHI-H8", 1.65, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_bopb_h3, "renderusdt", 10800, "RENDER-BOPB-H3", 1.75, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_ichi_h8, "pythusdt", 28800, "PYTH-ICHI-H8", 1.51, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_willr_h12, "arkmusdt", 43200, "ARKM-WILLR-H12", 1.51, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_d1, "tonusdt", 86400, "TON-TSMOM-D1", 2.43, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_tsmom_d2, "compusdt", 172800, "COMP-TSMOM-D2", 1.54, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_ichi_h8, "tonusdt", 28800, "TON-ICHI-H8", 2.24, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_ichi_h8, "sandusdt", 28800, "SAND-ICHI-H8", 1.71, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_ichi_h12, "tonusdt", 43200, "TON-ICHI-H12", 2.46, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_bopb_h6, "renderusdt", 21600, "RENDER-BOPB-H6", 1.55, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_dt_h12, "sandusdt", 43200, "SAND-DT-H12", 1.50, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_bopb_h2, "pythusdt", 7200, "PYTH-BOPB-H2", 1.65, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_tsmom_d2, "tonusdt", 172800, "TON-TSMOM-D2", 2.35, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_IMX, &s40_imxusdt_bopb_h4, "imxusdt", 14400, "IMX-BOPB-H4", 1.52, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_bopb_h4, "manausdt", 14400, "MANA-BOPB-H4", 1.74, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_dch_h4, "pythusdt", 14400, "PYTH-DCH-H4", 1.60, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_ARKM, &s40_arkmusdt_boll_h3, "arkmusdt", 10800, "ARKM-BOLL-H3", 1.95, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_PYTH, &s40_pythusdt_bopb_h3, "pythusdt", 10800, "PYTH-BOPB-H3", 1.56, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_MANA, &s40_manausdt_bopb_h6, "manausdt", 21600, "MANA-BOPB-H6", 1.55, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_dch_h6, "sandusdt", 21600, "SAND-DCH-H6", 1.50, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_RENDER, &s40_renderusdt_boll_h3, "renderusdt", 10800, "RENDER-BOLL-H3", 1.59, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_SAND, &s40_sandusdt_bopb_h3, "sandusdt", 10800, "SAND-BOPB-H3", 1.56, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_bopb_h4, "tonusdt", 14400, "TON-BOPB-H4", 2.52, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_bopb_h3, "tonusdt", 10800, "TON-BOPB-H3", 1.64, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_TON, &s40_tonusdt_bopb_h2, "tonusdt", 7200, "TON-BOPB-H2", 1.62, 0.0, 100, 730, 40});
g_slots.push_back({chimera::SYM_COMP, &s40_compusdt_kelt_h4, "compusdt", 14400, "COMP-KELT-H4", 1.50, 0.0, 100, 730, 40});
