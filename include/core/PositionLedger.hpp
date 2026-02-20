#pragma once
#include <unordered_map>
#include "core/Symbol.hpp"

namespace chimera {

class PositionLedger {
public:
    void apply_trade(SymbolID id, bool is_buy, double size) {
        if (is_buy) {
            positions_[id] += size;
        } else {
            positions_[id] -= size;
        }
    }
    
    double get_position(SymbolID id) const {
        auto it = positions_.find(id);
        return (it != positions_.end()) ? it->second : 0.0;
    }
    
    int num_positions() const {
        int count = 0;
        for (const auto& p : positions_) {
            if (p.second != 0.0) count++;
        }
        return count;
    }
    
private:
    std::unordered_map<SymbolID, double> positions_;
};

}
