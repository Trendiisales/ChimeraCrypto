#pragma once
#include <cstdio>
#include <cstring>

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
    
    char json_buffer[2048];
    
    const char* to_json() {
        snprintf(json_buffer, sizeof(json_buffer),
            "{\"equity\":%.2f,"
            "\"pnl\":%.2f,"
            "\"unrealized_pnl\":%.2f,"
            "\"day_pnl\":%.2f,"
            "\"latency_ms\":%.3f,"
            "\"orders_sent\":%d,"
            "\"fills_received\":%d,"
            "\"positions\":%d,"
            "\"governor\":\"%s\","
            "\"kill_switch\":%s,"
            "\"exposure_usd\":%.2f,"
            "\"win_rate\":%.4f,"
            "\"sharpe_ratio\":%.2f,"
            "\"trades_today\":%d,"
            "\"uptime_hours\":%.2f,"
            "\"mode\":\"%s\","
            "\"healthy\":%s,"
            "\"btc_price\":%.2f,"
            "\"eth_price\":%.2f,"
            "\"sol_price\":%.2f,"
            "\"btc_change_pct\":%.4f,"
            "\"eth_change_pct\":%.4f,"
            "\"sol_change_pct\":%.4f,"
            "\"last_order_symbol\":\"%s\","
            "\"last_order_side\":\"%s\","
            "\"last_order_size\":%.4f,"
            "\"last_order_price\":%.2f,"
            "\"last_order_usd\":%.2f,"
            "\"last_order_signal\":%.2f,"
            "\"last_order_conviction\":%.2f,"
            "\"last_order_cost_floor\":%.2f,"
            "\"last_order_time\":\"%s\","
            "\"btc_position\":%.4f,"
            "\"eth_position\":%.4f,"
            "\"sol_position\":%.4f,"
            "\"orders_blocked\":%d}",
            equity, pnl, unrealized_pnl, day_pnl, latency_ms,
            orders_sent, fills_received, positions, governor,
            kill_switch ? "true" : "false", exposure_usd,
            win_rate, sharpe_ratio, trades_today, uptime_hours,
            mode, healthy ? "true" : "false",
            btc_price, eth_price, sol_price,
            btc_change_pct, eth_change_pct, sol_change_pct,
            last_order_symbol, last_order_side,
            last_order_size, last_order_price, last_order_usd,
            last_order_signal, last_order_conviction, last_order_cost_floor,
            last_order_time, btc_position, eth_position, sol_position,
            orders_blocked);
        return json_buffer;
    }
};

}
