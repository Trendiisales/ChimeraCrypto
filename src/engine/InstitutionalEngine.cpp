#include "engine/InstitutionalEngine.hpp"
#include <cmath>
#include <algorithm>

namespace chimera {

InstitutionalEngine::InstitutionalEngine(double initial_capital)
    : equity_(initial_capital)
    , risk_governor_(initial_capital)
    , shadow_executor_(false, risk_governor_, position_ledger_, order_tracker_)
{
}

InstitutionalEngine::~InstitutionalEngine() = default;

void InstitutionalEngine::update_book(const std::string& symbol, 
                                     double bid, double ask,
                                     double bid_size, double ask_size)
{
}

void InstitutionalEngine::tick(const std::string& symbol)
{
}

double InstitutionalEngine::get_equity() const 
{ 
    return equity_; 
}

const char* InstitutionalEngine::get_governor_state() const 
{ 
    return "ACTIVE"; 
}

}
