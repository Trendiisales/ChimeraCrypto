#include "execution/MultiVenueExecutor.hpp"

namespace chimera {

void MultiVenueExecutor::start_all()
{
    // No venues to start - FIX removed
}

void MultiVenueExecutor::stop_all()
{
    // No venues to stop - FIX removed
}

bool MultiVenueExecutor::send_order(
    const std::string& clordid,
    const std::string& symbol,
    double qty,
    double expected_price,
    bool is_buy,
    const std::string& raw)
{
    // FIX removed - orders go through REST API
    return false;
}

SmartRouter& MultiVenueExecutor::router()
{
    return router_;
}

}
