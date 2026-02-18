#pragma once
#include <map>
#include <vector>
#include <cstdint>
#include <functional>
#include "L2Types.hpp"

namespace chimera {

enum class L2ValidationMode {
    STRICT,    // Bootstrap/replay: enforce overlap rule
    RELAXED    // Live: monotonic only
};

class L2Book {
public:
    L2Book();

    void clear();

    void load_snapshot(uint64_t last_update_id,
                       const std::vector<L2Level>& bids,
                       const std::vector<L2Level>& asks);

    bool apply_delta(uint64_t U, uint64_t u,
                     const std::vector<L2Level>& bids,
                     const std::vector<L2Level>& asks,
                     L2ValidationMode mode);

    bool is_synced() const { return synced_; }
    uint64_t last_update_id() const { return last_update_id_; }

    double best_bid() const;
    double best_ask() const;
    double mid() const;
    double imbalance_top5() const;

private:
    std::map<double, double, std::greater<double>> bids_;
    std::map<double, double> asks_;

    uint64_t last_update_id_;
    bool synced_;
};

}
