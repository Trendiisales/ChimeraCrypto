#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "routing/SmartRouter.hpp"

namespace chimera {

// MultiVenueExecutor - FIX protocol removed, using REST APIs only
class MultiVenueExecutor {
public:
    void start_all();
    void stop_all();

    bool send_order(const std::string& clordid,
                    const std::string& symbol,
                    double qty,
                    double expected_price,
                    bool is_buy,
                    const std::string& raw);

    SmartRouter& router();

private:
    SmartRouter router_;
};

}
