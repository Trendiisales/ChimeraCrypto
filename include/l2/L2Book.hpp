#pragma once
#include <vector>
#include <cstdint>
#include "L2Types.hpp"

namespace chimera {

class L2Book {
public:
    void load_snapshot(const Snapshot& snap, uint64_t last_update_id);
    bool apply_delta(const DepthEvent& ev);

    const std::vector<Level>& bids() const;
    const std::vector<Level>& asks() const;

    bool ready() const;
    bool needs_resync() const;

private:
    std::vector<Level> bids_;
    std::vector<Level> asks_;

    uint64_t last_update_id_ = 0;
    bool ready_ = false;
    bool resync_required_ = false;

    void apply_levels(std::vector<Level>& side,
                      const std::vector<Level>& updates);
};

}
