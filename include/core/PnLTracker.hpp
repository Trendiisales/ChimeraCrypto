#pragma once
#include <unordered_map>
#include "core/Symbol.hpp"

namespace chimera {

struct PositionPnL {
    double qty = 0.0;
    double avg_entry = 0.0;
    double realized_pnl = 0.0;
    double unrealized_pnl = 0.0;
};

class PnLTracker {
public:
    void update_trade(SymbolID id, bool is_buy, double qty, double price);
    void mark_to_market(SymbolID id, double current_price);
    
    PositionPnL get_position_pnl(SymbolID id) const;
    double get_total_realized_pnl() const;
    double get_total_unrealized_pnl() const;
    double get_total_pnl() const;
    
private:
    std::unordered_map<SymbolID, PositionPnL> positions_;
};

}
