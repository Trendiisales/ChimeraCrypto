#pragma once
#include <string>
#include <variant>
#include <cstdint>

namespace chimera {

enum class EventType {
    MARKET_UPDATE,
    ORDER_EXECUTED,
    SYSTEM_ALERT
};

struct MarketEvent {
    std::string symbol;
    double bid;
    double ask;
};

struct ExecutionEvent {
    std::string symbol;
    double price;
    double qty;
};

struct SystemEvent {
    std::string reason;
};

using EventPayload = std::variant<
    MarketEvent,
    ExecutionEvent,
    SystemEvent
>;

struct Event {
    EventType type;
    EventPayload payload;
    uint64_t sequence;
};

}
