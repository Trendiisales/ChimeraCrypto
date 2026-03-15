#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include "config/TradingConfig.hpp"
#include "live/BinanceWSFeed.hpp"

namespace chimera {

enum class LimitStatus {
    IDLE,
    PENDING,
    FILLED,
    CANCELLED
};

enum class LimitFillMode {
    NONE,
    BOOK_CROSS,
    TRADE_AGGRESSION
};

struct LimitOrder {
    LimitStatus status      = LimitStatus::IDLE;
    double      limit_price = 0.0;
    double      fill_price  = 0.0;
    int64_t     posted_ts   = 0;
    int64_t     timeout_ms  = 0;
    double      stale_bp    = 0.0;
    int         layer_id    = 0;
    bool        is_buy      = true;
    LimitFillMode fill_mode = LimitFillMode::NONE;
};

class LimitOrderManager {
public:
    void enter_pending(int layer_id, double bid, double ask, int64_t ts, bool is_buy = true) {
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
        order_.is_buy      = is_buy;
        order_.fill_mode   = LimitFillMode::NONE;

        std::printf("[LIMIT-POST] layer=%d | bid=%.8f | ask=%.8f | limit=%.8f | spread_bp=%.2f | timeout=%ldms\n",
            layer_id, bid, ask, limit_px,
            mid > 0.0 ? (ask - bid) / mid * 10000.0 : 0.0,
            static_cast<long>(timeout));
        std::fflush(stdout);
    }

    LimitStatus update(const MarketTick& tick, int64_t ts) {
        return update(tick.ask,
                      tick.bid,
                      tick.last_price,
                      tick.agg_buy_volume,
                      tick.agg_sell_volume,
                      ts);
    }

    LimitStatus update(double ask, double bid, int64_t ts) {
        return update(ask, bid, 0.0, 0.0, 0.0, ts);
    }

    LimitStatus update(double ask,
                       double bid,
                       double trade_price,
                       double agg_buy_volume,
                       double agg_sell_volume,
                       int64_t ts) {
        if (order_.status != LimitStatus::PENDING) {
            return order_.status;
        }

        order_.fill_mode = LimitFillMode::NONE;

        if (crosses_book(ask, bid)) {
            order_.fill_price  = order_.limit_price;
            order_.status      = LimitStatus::FILLED;
            order_.fill_mode   = LimitFillMode::BOOK_CROSS;
            const int64_t wait_ms = ts - order_.posted_ts;
            std::printf("[LIMIT-FILL] limit=%.8f | ask=%.8f | wait=%ldms\n",
                order_.limit_price, ask, static_cast<long>(wait_ms));
            std::fflush(stdout);
            return order_.status;
        }

        if (fills_from_trade_flow(bid, ask, trade_price, agg_buy_volume, agg_sell_volume)) {
            order_.fill_price  = order_.limit_price;
            order_.status      = LimitStatus::FILLED;
            order_.fill_mode   = LimitFillMode::TRADE_AGGRESSION;
            const int64_t wait_ms = ts - order_.posted_ts;
            std::printf("[LIMIT-FILL-TRADE] limit=%.8f | bid=%.8f | ask=%.8f | trade=%.8f | wait=%ldms\n",
                order_.limit_price, bid, ask, trade_price, static_cast<long>(wait_ms));
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
    LimitFillMode     fill_mode() const { return order_.fill_mode; }
    const char*       fill_mode_name() const {
        switch (order_.fill_mode) {
            case LimitFillMode::BOOK_CROSS: return "book_cross";
            case LimitFillMode::TRADE_AGGRESSION: return "trade_aggression";
            default: return "none";
        }
    }

private:
    bool crosses_book(double ask, double bid) const {
        if (order_.is_buy) {
            return ask > 0.0 && ask <= order_.limit_price;
        }
        return bid > 0.0 && bid >= order_.limit_price;
    }

    bool fills_from_trade_flow(double bid,
                               double ask,
                               double trade_price,
                               double agg_buy_volume,
                               double agg_sell_volume) const {
        if (order_.limit_price <= 0.0 || trade_price <= 0.0) return false;

        const double tolerance = std::max(1e-8, order_.limit_price * 1e-9);
        if (order_.is_buy) {
            const bool at_or_better_bid = bid > 0.0 && bid + tolerance >= order_.limit_price;
            const bool seller_aggressed = agg_sell_volume > 0.0;
            const bool trade_hit_level = trade_price <= order_.limit_price + tolerance;
            return at_or_better_bid && seller_aggressed && trade_hit_level;
        }

        const bool at_or_better_ask = ask > 0.0 && ask - tolerance <= order_.limit_price;
        const bool buyer_aggressed = agg_buy_volume > 0.0;
        const bool trade_hit_level = trade_price + tolerance >= order_.limit_price;
        return at_or_better_ask && buyer_aggressed && trade_hit_level;
    }

    LimitOrder order_;
};

} // namespace chimera
