#pragma once
#include <cstdint>
#include <string>

namespace chimera {

enum class ExecType {
    NEW_ACK,
    PARTIAL_FILL,
    FILL,
    CANCEL_ACK,
    REJECT
};

struct ExecutionEvent {
    std::string orderId;
    ExecType type;
    
    double price = 0.0;
    double qty = 0.0;
    
    uint64_t sendTs = 0;
    uint64_t ackTs = 0;
    uint64_t fillTs = 0;
    
    double slippage = 0.0;
    double rttMs = 0.0;
    bool rejected = false;
};

} // namespace chimera
