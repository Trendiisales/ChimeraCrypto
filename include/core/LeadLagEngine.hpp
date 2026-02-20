#pragma once
#include <cstdint>
#include <cmath>

namespace chimera {

struct LeadLagState {
    double btc_price_250ms_ago;
    double eth_price_250ms_ago;
    double sol_price_250ms_ago;
    int64_t last_update_ms;
    
    void reset() {
        btc_price_250ms_ago = 0.0;
        eth_price_250ms_ago = 0.0;
        sol_price_250ms_ago = 0.0;
        last_update_ms = 0;
    }
};

class LeadLagEngine {
public:
    LeadLagEngine() { state_.reset(); }
    
    void update_price(int symbol_id, double price, int64_t now_ms) {
        if (now_ms - state_.last_update_ms >= 250) {
            state_.btc_price_250ms_ago = btc_current_;
            state_.eth_price_250ms_ago = eth_current_;
            state_.sol_price_250ms_ago = sol_current_;
            state_.last_update_ms = now_ms;
        }
        
        if (symbol_id == 0) btc_current_ = price;
        else if (symbol_id == 1) eth_current_ = price;
        else if (symbol_id == 2) sol_current_ = price;
    }
    
    bool check_eth_signal(double latency_p95, bool btc_expand_active, int& direction) {
        if (btc_expand_active) return false;
        if (latency_p95 > 8.0) return false;
        if (state_.btc_price_250ms_ago == 0.0 || state_.eth_price_250ms_ago == 0.0) return false;
        
        double btc_delta = (btc_current_ - state_.btc_price_250ms_ago) / state_.btc_price_250ms_ago * 10000.0;
        double eth_delta = (eth_current_ - state_.eth_price_250ms_ago) / state_.eth_price_250ms_ago * 10000.0;
        
        if (std::fabs(btc_delta) >= 12.0 && std::fabs(eth_delta) < 6.0) {
            direction = (btc_delta > 0) ? 1 : -1;
            return true;
        }
        return false;
    }
    
    bool check_sol_signal(double latency_p95, bool btc_expand_active, int& direction) {
        if (btc_expand_active) return false;
        if (latency_p95 > 8.0) return false;
        if (state_.btc_price_250ms_ago == 0.0 || state_.sol_price_250ms_ago == 0.0) return false;
        
        double btc_delta = (btc_current_ - state_.btc_price_250ms_ago) / state_.btc_price_250ms_ago * 10000.0;
        double sol_delta = (sol_current_ - state_.sol_price_250ms_ago) / state_.sol_price_250ms_ago * 10000.0;
        
        if (std::fabs(btc_delta) >= 12.0 && std::fabs(sol_delta) < 6.0) {
            direction = (btc_delta > 0) ? 1 : -1;
            return true;
        }
        return false;
    }

private:
    LeadLagState state_;
    double btc_current_ = 0.0;
    double eth_current_ = 0.0;
    double sol_current_ = 0.0;
};

}
