#pragma once
#include <cstdint>

namespace chimera {

enum class TradingMode {
    ENABLE,
    POST_ONLY,
    TAKE_ONLY,
    REDUCE_SIZE,
    DISABLE,
    KILL
};

struct PolicyState {
    TradingMode mode = TradingMode::ENABLE;
    
    double latencyMs = 0.0;
    double avgSlippage = 0.0;
    double rejectRate = 0.0;
    
    bool feedHealthy = true;
    bool exchangeStable = true;
    
    const char* reason = "";
};

} // namespace chimera
