#pragma once

namespace chimera {

// ============================================================================
// CHIMERA TRADING ENGINE - CENTRAL CONFIGURATION
// ============================================================================
// 
// All tunable parameters in one place for easy adjustment.
// Modify values here, recompile, no need to hunt through multiple files.
//
// ============================================================================

struct TradingConfig {
    
    // ------------------------------------------------------------------------
    // REGIME CLASSIFICATION THRESHOLDS
    // ------------------------------------------------------------------------
    // Controls when market transitions between DEAD/GRIND/BUILDUP/BREAKOUT
    // Uses HYSTERESIS BANDS - different thresholds for entering vs exiting regimes
    // This prevents thrashing at boundaries
    
    // Minimum long_avg to avoid division by zero
    static constexpr double REGIME_MIN_LONG_AVG = 0.004;
    
    // DEAD REGIME
    static constexpr double REGIME_DEAD_ENTER = 0.60;   // Enter DEAD if ratio falls below this
    static constexpr double REGIME_DEAD_EXIT = 0.90;    // Exit DEAD only if ratio rises above this
    
    // GRIND REGIME
    static constexpr double REGIME_GRIND_ENTER_FROM_DEAD = 0.90;     // Enter GRIND from DEAD if ratio > this
    static constexpr double REGIME_GRIND_EXIT_TO_DEAD = 0.75;        // Exit GRIND to DEAD if ratio < this
    static constexpr double REGIME_GRIND_EXIT_TO_BUILDUP = 1.55;     // Exit GRIND to BUILDUP if ratio > this (matches BUILDUP_ENTER)
    
    // BUILDUP REGIME
    // Widened bands - BUILDUP must survive normal pullbacks
    // Enter at 1.55 (not 1.45) and exit only at 0.95 (not 1.10)
    // This creates 0.60 point separation for real inertia
    static constexpr double REGIME_BUILDUP_ENTER = 1.55;    // Enter BUILDUP if ratio rises above this
    static constexpr double REGIME_BUILDUP_EXIT = 0.95;     // Exit BUILDUP to GRIND only if ratio falls below this
    static constexpr double REGIME_BUILDUP_TO_BREAKOUT = 1.95;  // Enter BREAKOUT from BUILDUP if ratio > this
    
    // BREAKOUT REGIME
    static constexpr double REGIME_BREAKOUT_ENTER = 1.95;   // Enter BREAKOUT if ratio rises above this
    static constexpr double REGIME_BREAKOUT_EXIT = 1.55;    // Exit BREAKOUT to BUILDUP only if ratio falls below this
    
    // REGIME STABILITY - Minimum ticks before regime can change again
    static constexpr int MIN_REGIME_TICKS = 30;
    
    
    // ------------------------------------------------------------------------
    // BREAKOUT LAYER (IMPULSE) SETTINGS
    // ------------------------------------------------------------------------
    
    // Minimum ticks in short window to fire impulse signal
    static constexpr int IMPULSE_MIN_SHORT_TICKS = 5;
    
    // TRADE STABILITY - Minimum ticks to hold position before allowing exit
    // Raised from 15 to 50 after observing 30ms hold times = pure spread scalping
    // This prevents exiting on microstructure noise before move develops
    static constexpr int MIN_HOLD_TICKS = 50;
    
    
    // ------------------------------------------------------------------------
    // EXPANSION LAYER SETTINGS
    // ------------------------------------------------------------------------
    
    // Volatility expansion ratio threshold
    // (short_vol / long_vol must exceed this)
    static constexpr double EXPANSION_VOL_RATIO = 1.125;  // Was 1.5 (reduced 25%)
    
    // Minimum ticks in short window for expansion
    static constexpr int EXPANSION_MIN_SHORT_TICKS = 8;
    
    
    // ------------------------------------------------------------------------
    // VOLATILITY CALCULATION
    // ------------------------------------------------------------------------
    
    // Rolling window sizes for volatility calculation
    static constexpr int SHORT_VOL_WINDOW = 20;    // Short volatility window (ticks)
    static constexpr int LONG_VOL_WINDOW = 200;    // Long volatility window (ticks) - DEPRECATED, using EMA now
    
    // EMA smoothing factor for long volatility baseline
    // Alpha = 0.06 means ~17-tick half-life (20% faster than 0.05)
    // Reduced lag helps catch early breakouts without suppressing them
    // Was 0.05 (~20-tick) - slightly too laggy, causing early low_long_vol rejections
    static constexpr double LONG_VOL_EMA_ALPHA = 0.06;
    
    // EMA smoothing factor for vol_ratio itself
    // This reduces tick-to-tick noise in regime classification
    // Alpha = 0.12 gives ~15-20 tick memory
    static constexpr double VOL_RATIO_EMA_ALPHA = 0.12;
    
    // Minimum long_vol to avoid division by zero
    static constexpr double VOL_MIN_LONG = 1e-8;
    
    // ------------------------------------------------------------------------
    // VOLATILITY FLOOR GATE - COST PREVENTION
    // ------------------------------------------------------------------------
    // This is the STRUCTURAL FILTER that prevents trading when market movement
    // is smaller than transaction costs. If long_vol is too low, even perfect
    // entries cannot beat the 2bp spread cost.
    //
    // This prevents trading in regimes where:
    //   theoretical_move < slippage_cost (2bp)
    //   → guarantees negative EV
    //
    // Raised from 0.000004 to 0.000010 after observing:
    //   - Win rate 100% but still losing (1.83bp capture vs 2bp cost)
    //   - Trading in microscopic vol regimes (0.000001-0.000003)
    //   - Need 4-5bp moves minimum to beat costs
    //
    // This is NOT vol_ratio. This is absolute volatility floor.
    static constexpr double MIN_LONG_VOL_FOR_TRADING = 0.000010;
    
    
    // ------------------------------------------------------------------------
    // LATENCY LIMITS
    // ------------------------------------------------------------------------
    
    // Hard cutoff - reject if latency exceeds this
    // Calibrated: Tokyo VPS -> Binance AWS Tokyo
    //   WS p95 = 18-25ms | REST RTT = 36-38ms | Clock offset ~18ms
    //   HARD LIMIT = 50ms: normal + 2x buffer. Above = genuine congestion.
    static constexpr double LATENCY_HARD_LIMIT_MS = 50.0;

    // NET_CLEAN threshold: below = full size, above = reduced size
    static constexpr double LATENCY_NET_CLEAN_MS  = 30.0;

    // Lead-lag max: requires fast execution to capture BTC->ETH/SOL propagation
    static constexpr double LATENCY_LEADLAG_MAX_MS = 35.0;
    
    
    // ------------------------------------------------------------------------
    // ENTRY CONFIRMATION
    // ------------------------------------------------------------------------
    
    // Minimum price displacement required before allowing entry
    // Price must move at least (MIN_DISPLACEMENT_LONG_MULT * long_vol) from regime anchor
    // This prevents entering on volatility expansion WITHOUT directional displacement
    static constexpr double MIN_DISPLACEMENT_LONG_MULT = 0.75;
    
    
    // ------------------------------------------------------------------------
    // EXIT MANAGEMENT
    // ------------------------------------------------------------------------
    
    // Volatility-normalized trailing stop
    // Trail distance = TRAIL_LONG_VOL_MULT * long_vol
    static constexpr double TRAIL_LONG_VOL_MULT = 0.75;
    
    // Minimum profit (in bp) before trailing stop activates
    // Raised from 1.0bp to 2.5bp - don't trail until we've beaten costs
    // This prevents early exits on micro-moves that can't cover spread
    static constexpr double MIN_PROFIT_TO_TRAIL_BP = 2.5;
    
    
    // ------------------------------------------------------------------------
    // DIAGNOSTIC OUTPUT
    // ------------------------------------------------------------------------
    
    // Print regime diagnostics every N ticks
    static constexpr int REGIME_DIAG_INTERVAL = 500;
    
    // Print symbol state every N ticks  
    static constexpr int SYMBOL_STATE_INTERVAL = 500;
};

} // namespace chimera
