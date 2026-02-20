#include "core/PnLTracker.hpp"
#include <cmath>

namespace chimera {

void PnLTracker::update_trade(SymbolID id, bool is_buy, double qty, double price) {
    auto& pos = positions_[id];
    
    double trade_qty = is_buy ? qty : -qty;
    
    // Check if closing/reducing position
    if ((pos.qty > 0 && !is_buy) || (pos.qty < 0 && is_buy)) {
        double close_qty = std::min(std::abs(trade_qty), std::abs(pos.qty));
        
        // Realize P&L on closed portion
        if (pos.qty > 0) {
            // Closing long
            pos.realized_pnl += close_qty * (price - pos.avg_entry);
        } else {
            // Closing short
            pos.realized_pnl += close_qty * (pos.avg_entry - price);
        }
        
        // Update position
        pos.qty += trade_qty;
        
        // If flipping, set new avg entry
        if ((pos.qty > 0 && !is_buy) || (pos.qty < 0 && is_buy)) {
            pos.avg_entry = price;
        }
    } else {
        // Opening or adding to position
        double old_notional = pos.qty * pos.avg_entry;
        double new_notional = trade_qty * price;
        pos.qty += trade_qty;
        
        if (pos.qty != 0) {
            pos.avg_entry = (old_notional + new_notional) / pos.qty;
        } else {
            pos.avg_entry = 0;
        }
    }
}

void PnLTracker::mark_to_market(SymbolID id, double current_price) {
    auto& pos = positions_[id];
    
    if (pos.qty == 0) {
        pos.unrealized_pnl = 0;
        return;
    }
    
    if (pos.qty > 0) {
        // Long position
        pos.unrealized_pnl = pos.qty * (current_price - pos.avg_entry);
    } else {
        // Short position
        pos.unrealized_pnl = std::abs(pos.qty) * (pos.avg_entry - current_price);
    }
}

PositionPnL PnLTracker::get_position_pnl(SymbolID id) const {
    auto it = positions_.find(id);
    return (it != positions_.end()) ? it->second : PositionPnL();
}

double PnLTracker::get_total_realized_pnl() const {
    double total = 0;
    for (const auto& p : positions_) {
        total += p.second.realized_pnl;
    }
    return total;
}

double PnLTracker::get_total_unrealized_pnl() const {
    double total = 0;
    for (const auto& p : positions_) {
        total += p.second.unrealized_pnl;
    }
    return total;
}

double PnLTracker::get_total_pnl() const {
    return get_total_realized_pnl() + get_total_unrealized_pnl();
}

}
