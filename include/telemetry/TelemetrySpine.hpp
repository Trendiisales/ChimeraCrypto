#pragma once
#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>
#include "telemetry/DeskSnapshot.hpp"

namespace chimera {

class TelemetrySpine {
public:
    TelemetrySpine() = default;

    void publish(DeskSnapshot* snap) {
        snapshot_.store(snap, std::memory_order_release);
    }

    std::string json() const {
        DeskSnapshot* s = snapshot_.load(std::memory_order_acquire);
        DeskSnapshot fallback;
        if (!s) s = &fallback;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        
        oss << "{"
            << "\"equity\":" << s->equity << ","
            << "\"pnl\":" << s->pnl << ","
            << "\"unrealized_pnl\":" << s->unrealized_pnl << ","
            << "\"day_pnl\":" << s->day_pnl << ","
            << "\"latency_ms\":" << s->latency_ms << ","
            << "\"orders_sent\":" << s->orders_sent << ","
            << "\"fills_received\":" << s->fills_received << ","
            << "\"positions\":" << s->positions << ","
            << "\"governor\":\"" << (s->governor ? s->governor : "") << "\","
            << "\"kill_switch\":" << (s->kill_switch ? "true" : "false") << ","
            << "\"exposure_usd\":" << s->exposure_usd << ","
            << "\"win_rate\":" << s->win_rate << ","
            << "\"sharpe_ratio\":" << s->sharpe_ratio << ","
            << "\"trades_today\":" << s->trades_today << ","
            << "\"uptime_hours\":" << s->uptime_hours << ","
            << "\"mode\":\"" << (s->mode ? s->mode : "") << "\","
            << "\"healthy\":" << (s->healthy ? "true" : "false") << ","
            << "\"btc_price\":" << s->btc_price << ","
            << "\"eth_price\":" << s->eth_price << ","
            << "\"sol_price\":" << s->sol_price << ","
            << "\"btc_change_pct\":" << s->btc_change_pct << ","
            << "\"eth_change_pct\":" << s->eth_change_pct << ","
            << "\"sol_change_pct\":" << s->sol_change_pct << ","
            << "\"last_order_symbol\":\"" << (s->last_order_symbol ? s->last_order_symbol : "") << "\","
            << "\"last_order_side\":\"" << (s->last_order_side ? s->last_order_side : "") << "\","
            << "\"last_order_size\":" << s->last_order_size << ","
            << "\"last_order_price\":" << s->last_order_price << ","
            << "\"last_order_usd\":" << s->last_order_usd << ","
            << "\"last_order_signal\":" << s->last_order_signal << ","
            << "\"last_order_conviction\":" << s->last_order_conviction << ","
            << "\"last_order_cost_floor\":" << s->last_order_cost_floor << ","
            << "\"last_order_time\":\"" << (s->last_order_time ? s->last_order_time : "") << "\","
            << "\"btc_position\":" << s->btc_position << ","
            << "\"eth_position\":" << s->eth_position << ","
            << "\"sol_position\":" << s->sol_position << ","
            << "\"orders_blocked\":" << s->orders_blocked
            << "}";

        return oss.str();
    }

private:
    std::atomic<DeskSnapshot*> snapshot_{nullptr};
};

}
