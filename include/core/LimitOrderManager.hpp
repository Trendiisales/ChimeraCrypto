#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include "config/TradingConfig.hpp"

namespace chimera {

enum class LimitStatus {
    IDLE,
    PENDING,
    FILLED,
    CANCELLED
};

struct LimitOrder {
    LimitStatus status      = LimitStatus::IDLE;
    double      limit_price = 0.0;
    double      fill_price  = 0.0;
    int64_t     posted_ts   = 0;
    int64_t     timeout_ms  = 0;
    double      stale_bp    = 0.0;
    int         layer_id    = 0;
};

class LimitOrderManager {
public:
    void enter_pending(int layer_id, double bid, double ask, int64_t ts) {
        const double spread = std::max(ask - bid, 0.0);
        const double mid = (bid + ask) * 0.5;
        const double price_tick = std::max(mid * 0.000001, 0.00000001);

        double join_ratio = 0.0;
        int64_t timeout = TradingConfig::MAKER_IMBALANCE_TIMEOUT_MS;
        double stale_bp = TradingConfig::MAKER_STALE_BP;

        switch (layer_id) {
            case 0:
                join_ratio = 0.00;
                timeout = TradingConfig::MAKER_IMBALANCE_TIMEOUT_MS;
                stale_bp = TradingConfig::MAKER_STALE_BP;
                break;
            case 1:
                join_ratio = 0.15;
                timeout = TradingConfig::MAKER_IMPULSE_TIMEOUT_MS;
                stale_bp = TradingConfig::MAKER_STALE_BP;
                break;
            case 2:
                join_ratio = 0.25;
                timeout = TradingConfig::MAKER_IMPULSE_TIMEOUT_MS;
                stale_bp = TradingConfig::MAKER_STALE_BP;
                break;
            case 3:
                join_ratio = 0.35;
                timeout = TradingConfig::MAKER_LEADLAG_TIMEOUT_MS;
                stale_bp = TradingConfig::MAKER_STALE_BP;
                break;
            case 4:
                join_ratio = 0.45;
                timeout = 300;
                stale_bp = 2.0;
                break;
            default:
                join_ratio = 0.20;
                timeout = TradingConfig::MAKER_IMPULSE_TIMEOUT_MS;
                stale_bp = TradingConfig::MAKER_STALE_BP;
                break;
        }

        double limit_px = bid;
        if (spread > price_tick) {
            limit_px = bid + spread * join_ratio;
            limit_px = std::min(limit_px, ask - price_tick);
            limit_px = std::max(limit_px, bid);
        }

        order_.status      = LimitStatus::PENDING;
        order_.limit_price = limit_px;
        order_.fill_price  = 0.0;
        order_.posted_ts   = ts;
        order_.timeout_ms  = timeout;
        order_.stale_bp    = stale_bp;
        order_.layer_id    = layer_id;

        std::printf("[LIMIT-POST] layer=%d | bid=%.8f | ask=%.8f | limit=%.8f | spread_bp=%.2f | timeout=%ldms\n",
            layer_id, bid, ask, limit_px,
            mid > 0.0 ? (ask - bid) / mid * 10000.0 : 0.0,
            static_cast<long>(timeout));
        std::fflush(stdout);
    }

    LimitStatus update(double ask, double bid, int64_t ts) {
        (void)bid;
        if (order_.status != LimitStatus::PENDING) {
            return order_.status;
        }

        if (ask <= order_.limit_price) {
            order_.fill_price  = order_.limit_price;
            order_.status      = LimitStatus::FILLED;
            const int64_t wait_ms = ts - order_.posted_ts;
            std::printf("[LIMIT-FILL] limit=%.8f | ask=%.8f | wait=%ldms\n",
                order_.limit_price, ask, static_cast<long>(wait_ms));
            std::fflush(stdout);
            return order_.status;
        }

        const double ask_above_bp = order_.limit_price > 0.0
            ? (ask - order_.limit_price) / order_.limit_price * 10000.0
            : 0.0;
        if (ask_above_bp > order_.stale_bp) {
            order_.status = LimitStatus::CANCELLED;
            std::printf("[LIMIT-STALE] limit=%.8f | ask=%.8f | drift=%.2fbp > %.2fbp\n",
                order_.limit_price, ask, ask_above_bp, order_.stale_bp);
            std::fflush(stdout);
            return order_.status;
        }

        if (ts - order_.posted_ts > order_.timeout_ms) {
            order_.status = LimitStatus::CANCELLED;
            std::printf("[LIMIT-TIMEOUT] limit=%.8f | waited=%ldms\n",
                order_.limit_price, static_cast<long>(order_.timeout_ms));
            std::fflush(stdout);
            return order_.status;
        }

        return order_.status;
    }

    void cancel() {
        if (order_.status == LimitStatus::PENDING) {
            order_.status = LimitStatus::CANCELLED;
        }
    }

    void reset() { order_ = LimitOrder{}; }

    const LimitOrder& order() const { return order_; }
    LimitStatus       status() const { return order_.status; }
    double            fill_price() const { return order_.fill_price; }
    bool              pending() const { return order_.status == LimitStatus::PENDING; }

private:
    LimitOrder order_;
};

} // namespace chimera
