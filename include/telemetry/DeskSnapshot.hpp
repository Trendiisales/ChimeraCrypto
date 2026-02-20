#pragma once
#include <string>

namespace chimera {

struct DeskSnapshot {
    double equity = 0.0;
    double pnl = 0.0;
    double unrealized_pnl = 0.0;
    double day_pnl = 0.0;
    double drawdown = 0.0;
    
    double latency_ms = 0.0;
    double avg_slippage_bps = 0.0;
    int orders_sent = 0;
    int fills_received = 0;
    int rejects = 0;
    int positions = 0;
    
    double exposure_usd = 0.0;
    const char* governor = "WARMING";
    bool kill_switch = false;
    
    double sharpe_ratio = 0.0;
    double win_rate = 0.0;
    int trades_today = 0;
    
    double uptime_hours = 0.0;
    const char* mode = "SHADOW";
    bool healthy = true;
    
    double btc_price = 0.0;
    double eth_price = 0.0;
    double sol_price = 0.0;
    double btc_change_pct = 0.0;
    double eth_change_pct = 0.0;
    double sol_change_pct = 0.0;
    
    const char* last_order_symbol = "";
    const char* last_order_side = "";
    double last_order_size = 0.0;
    double last_order_price = 0.0;
    double last_order_usd = 0.0;
    double last_order_signal = 0.0;
    double last_order_conviction = 0.0;
    double last_order_cost_floor = 0.0;
    const char* last_order_time = "";
    
    double btc_position = 0.0;
    double eth_position = 0.0;
    double sol_position = 0.0;
    
    int orders_blocked = 0;
};

}
