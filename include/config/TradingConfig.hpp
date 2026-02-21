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
    
    // Minimum long_avg to avoid division by zero
    static constexpr double REGIME_MIN_LONG_AVG = 0.004;
    
    // REGIME HYSTERESIS - Separate ENTER and EXIT thresholds to prevent thrashing
    static constexpr double REGIME_GRIND_ENTER = 0.60;      // Enter GRIND if score rises above this
    static constexpr double REGIME_GRIND_EXIT = 0.40;       // Exit GRIND only if score falls below this
    
    static constexpr double REGIME_BUILDUP_ENTER = 1.30;    // Enter BUILDUP if score rises above this
    static constexpr double REGIME_BUILDUP_EXIT = 1.10;     // Exit BUILDUP only if score falls below this
    
    static constexpr double REGIME_BREAKOUT_ENTER = 1.75;   // Enter BREAKOUT if score rises above this
    static constexpr double REGIME_BREAKOUT_EXIT = 1.55;    // Exit BREAKOUT only if score falls below this
    
    // REGIME STABILITY - Minimum ticks before regime can change again
    static constexpr int MIN_REGIME_TICKS = 30;
    
    
    // ------------------------------------------------------------------------
    // BREAKOUT LAYER (IMPULSE) SETTINGS
    // ------------------------------------------------------------------------
    
    // Minimum ticks in short window to fire impulse signal
    static constexpr int IMPULSE_MIN_SHORT_TICKS = 5;
    
    // TRADE STABILITY - Minimum ticks to hold position before allowing exit
    static constexpr int MIN_HOLD_TICKS = 15;
    
    
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
    static constexpr int LONG_VOL_WINDOW = 200;    // Long volatility window (ticks)
    
    // Minimum long_vol to avoid division by zero
    static constexpr double VOL_MIN_LONG = 1e-8;
    
    
    // ------------------------------------------------------------------------
    // LATENCY LIMITS
    // ------------------------------------------------------------------------
    
    // Hard cutoff - reject if latency exceeds this
    static constexpr double LATENCY_HARD_LIMIT_MS = 8.0;
    
    
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
    static constexpr double MIN_PROFIT_TO_TRAIL_BP = 1.0;
    
    
    // ------------------------------------------------------------------------
    // DIAGNOSTIC OUTPUT
    // ------------------------------------------------------------------------
    
    // Print regime diagnostics every N ticks
    static constexpr int REGIME_DIAG_INTERVAL = 500;
    
    // Print symbol state every N ticks  
    static constexpr int SYMBOL_STATE_INTERVAL = 500;
};

} // namespace chimera
