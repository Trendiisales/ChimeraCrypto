    chimera::EdgeEngine::Config eth_rsi_h8_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 5,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
    };
    chimera::EdgeEngine eth_rsi_h8(eth_rsi_h8_cfg);
    eth_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config bnb_rsi_h8_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
    };
    chimera::EdgeEngine bnb_rsi_h8(bnb_rsi_h8_cfg);
    bnb_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config doge_rsi_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.5,
    };
    chimera::EdgeEngine doge_rsi_h8(doge_rsi_h8_cfg);
    doge_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_rsi_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine xrp_rsi_h8(xrp_rsi_h8_cfg);
    xrp_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config apt_rsi_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine apt_rsi_h8(apt_rsi_h8_cfg);
    apt_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config sol_rsi_h8_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.6,
    };
    chimera::EdgeEngine sol_rsi_h8(sol_rsi_h8_cfg);
    sol_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config link_rsi_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine link_rsi_h8(link_rsi_h8_cfg);
    link_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config arb_rsi_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine arb_rsi_h8(arb_rsi_h8_cfg);
    arb_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config near_rsi_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 1.0,
    };
    chimera::EdgeEngine near_rsi_h8(near_rsi_h8_cfg);
    near_rsi_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config btc_boll_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine btc_boll_h8(btc_boll_h8_cfg);
    btc_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config eth_boll_h8_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine eth_boll_h8(eth_boll_h8_cfg);
    eth_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config sol_boll_h8_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 10,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine sol_boll_h8(sol_boll_h8_cfg);
    sol_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config bnb_boll_h8_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine bnb_boll_h8(bnb_boll_h8_cfg);
    bnb_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config avax_boll_h8_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine avax_boll_h8(avax_boll_h8_cfg);
    avax_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config link_boll_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine link_boll_h8(link_boll_h8_cfg);
    link_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_boll_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine xrp_boll_h8(xrp_boll_h8_cfg);
    xrp_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config doge_boll_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine doge_boll_h8(doge_boll_h8_cfg);
    doge_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config sui_boll_h8_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine sui_boll_h8(sui_boll_h8_cfg);
    sui_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config apt_boll_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
    };
    chimera::EdgeEngine apt_boll_h8(apt_boll_h8_cfg);
    apt_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config near_boll_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.8,
    };
    chimera::EdgeEngine near_boll_h8(near_boll_h8_cfg);
    near_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config arb_boll_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine arb_boll_h8(arb_boll_h8_cfg);
    arb_boll_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config eth_rsi_h16_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine eth_rsi_h16(eth_rsi_h16_cfg);
    eth_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config bnb_rsi_h16_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine bnb_rsi_h16(bnb_rsi_h16_cfg);
    bnb_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_rsi_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine xrp_rsi_h16(xrp_rsi_h16_cfg);
    xrp_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config link_rsi_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
    };
    chimera::EdgeEngine link_rsi_h16(link_rsi_h16_cfg);
    link_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config near_rsi_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.5,
    };
    chimera::EdgeEngine near_rsi_h16(near_rsi_h16_cfg);
    near_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config btc_rsi_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine btc_rsi_h16(btc_rsi_h16_cfg);
    btc_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config sol_rsi_h16_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine sol_rsi_h16(sol_rsi_h16_cfg);
    sol_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config doge_rsi_h16_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine doge_rsi_h16(doge_rsi_h16_cfg);
    doge_rsi_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config link_boll_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine link_boll_h16(link_boll_h16_cfg);
    link_boll_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_boll_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine xrp_boll_h16(xrp_boll_h16_cfg);
    xrp_boll_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config btc_boll_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine btc_boll_h16(btc_boll_h16_cfg);
    btc_boll_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config near_boll_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
    };
    chimera::EdgeEngine near_boll_h16(near_boll_h16_cfg);
    near_boll_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config eth_boll_h16_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine eth_boll_h16(eth_boll_h16_cfg);
    eth_boll_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_donch_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine xrp_donch_h8(xrp_donch_h8_cfg);
    xrp_donch_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config near_donch_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine near_donch_h8(near_donch_h8_cfg);
    near_donch_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config sui_donch_h8_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine sui_donch_h8(sui_donch_h8_cfg);
    sui_donch_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config btc_donch_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine btc_donch_h8(btc_donch_h8_cfg);
    btc_donch_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config arb_donch_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine arb_donch_h8(arb_donch_h8_cfg);
    arb_donch_h8.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_donch_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine xrp_donch_h16(xrp_donch_h16_cfg);
    xrp_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config bnb_donch_h16_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine bnb_donch_h16(bnb_donch_h16_cfg);
    bnb_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config btc_donch_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine btc_donch_h16(btc_donch_h16_cfg);
    btc_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config link_donch_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine link_donch_h16(link_donch_h16_cfg);
    link_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config sui_donch_h16_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine sui_donch_h16(sui_donch_h16_cfg);
    sui_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config near_donch_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine near_donch_h16(near_donch_h16_cfg);
    near_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config sol_donch_h16_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine sol_donch_h16(sol_donch_h16_cfg);
    sol_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config doge_donch_h16_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 30,
        .hold_bars      = 4,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.6,
    };
    chimera::EdgeEngine doge_donch_h16(doge_donch_h16_cfg);
    doge_donch_h16.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config bnb_donch_d2_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine bnb_donch_d2(bnb_donch_d2_cfg);
    bnb_donch_d2.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_donch_d2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 8,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };
    chimera::EdgeEngine xrp_donch_d2(xrp_donch_d2_cfg);
    xrp_donch_d2.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config btc_donch_d2_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.5,
    };
    chimera::EdgeEngine btc_donch_d2(btc_donch_d2_cfg);
    btc_donch_d2.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config eth_donch_d2_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine eth_donch_d2(eth_donch_d2_cfg);
    eth_donch_d2.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config link_donch_d2_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine link_donch_d2(link_donch_d2_cfg);
    link_donch_d2.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config btc_donch_d3_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 10,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine btc_donch_d3(btc_donch_d3_cfg);
    btc_donch_d3.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config eth_donch_d3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine eth_donch_d3(eth_donch_d3_cfg);
    eth_donch_d3.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config xrp_donch_d3_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
    };
    chimera::EdgeEngine xrp_donch_d3(xrp_donch_d3_cfg);
    xrp_donch_d3.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config bnb_donch_d3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine bnb_donch_d3(bnb_donch_d3_cfg);
    bnb_donch_d3.set_on_trade(on_trade_callback);

    chimera::EdgeEngine::Config doge_donch_d3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine doge_donch_d3(doge_donch_d3_cfg);
    doge_donch_d3.set_on_trade(on_trade_callback);