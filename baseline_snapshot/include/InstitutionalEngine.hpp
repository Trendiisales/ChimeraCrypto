#pragma once

#include <vector>
#include <string>
#include "ExecutionTracker.hpp"

namespace chimera {

class InstitutionalEngine {
public:
    explicit InstitutionalEngine(double initial_equity);

    void tick(size_t idx);

    void update_book(const std::string& symbol,
                     const std::vector<double>& bids,
                     const std::vector<double>& asks);

    void add_trade(const std::string& symbol,
                   double price,
                   double qty);

    double get_equity() const;

private:
    double equity_;
};

}
