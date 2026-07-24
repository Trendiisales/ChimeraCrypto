#pragma once
// ============================================================================
// SpotExecutor
//
// Owns the BinanceREST client and exposes the simple execute() interface
// that BalancedEngine calls.
//
// Loaded at startup from config/binance_credentials.json.
//
// In shadow mode (default): signs every order, logs it, never POSTs.
// In live mode:             signs and POSTs — real fills returned.
//
// execute() is async-safe — called from the WS feed thread.
// A mutex inside BinanceREST serialises concurrent calls.
// ============================================================================
#include "live/BinanceREST.hpp"
#include "live/ExchangeFilters.hpp"   // PRICE_FILTER tickSize snapping for protective stops
#include <string>
#include <atomic>
#include <cstdio>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <vector>

namespace chimera {

// Phase-1 review fix (2026-07-11): the ExecutionGateway is the ONLY caller
// permitted to reach execute(). Forward-declared here so it can be befriended;
// strategy code must route orders through ExecutionGatewayT::submit().
template <class E> class ExecutionGatewayT;

class SpotExecutor {
    // Only the execution gateway may call the raw execute() path.
    template <class E> friend class ExecutionGatewayT;
public:
    // -----------------------------------------------------------------------
    // init — must be called before execute().
    // Returns false if credentials file is missing or keys are invalid.
    // -----------------------------------------------------------------------
    bool init(const std::string& credentials_path = "config/binance_credentials.json") {
        std::vector<std::string> candidates;
        candidates.push_back(credentials_path);
        if (credentials_path == "config/binance_credentials.json") {
            // Allow launches from repo root, build/, and build/Release/.
            candidates.emplace_back("../config/binance_credentials.json");
            candidates.emplace_back("../../config/binance_credentials.json");
        }

        std::string loaded_path;
        bool loaded = false;
        bool attempted_existing = false;
        for (const auto& path : candidates) {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec)) continue;
            attempted_existing = true;
            if (rest_.load_credentials(path)) {
                loaded = true;
                loaded_path = path;
                break;
            }
        }

        if (!loaded) {
            if (!attempted_existing) {
                std::fprintf(stderr,
                             "[EXECUTOR] Credentials file not found. Tried: %s, %s, %s\n",
                             candidates.size() > 0 ? candidates[0].c_str() : "-",
                             candidates.size() > 1 ? candidates[1].c_str() : "-",
                             candidates.size() > 2 ? candidates[2].c_str() : "-");
            } else {
                std::fprintf(stderr, "[EXECUTOR] Failed to load credentials from available path(s)\n");
            }
            return false;
        }

        std::printf("[EXECUTOR] Credentials path: %s\n", loaded_path.c_str());
        std::fflush(stdout);

        // Fetch and log account balance to confirm API key is valid
        auto bal = rest_.get_account_balance();
        if (!bal.ok) {
            std::fprintf(stderr, "[EXECUTOR] Cannot fetch account balance — check API key permissions\n");
            return false;
        }

        std::printf("[EXECUTOR] Ready. shadow=%s\n",
                    rest_.is_shadow() ? "YES (paper trading)" : "NO (LIVE)");
        std::fflush(stdout);
        return true;
    }

    // free_balance — live free qty for a base asset (UPPER, e.g. "LTC"); -1.0 on
    // fetch failure. PUBLIC (read-only account query, not the befriended order
    // path). Used by the mimic live-mirror to clamp a stuck SELL to real coins
    // held (S-2026-07-19q). Safe no-op-ish when not ready (returns -1.0).
    double free_balance(const std::string& asset) {
        if (!rest_.is_ready()) return -1.0;
        return rest_.get_free_asset(asset);
    }

private:
    // -----------------------------------------------------------------------
    // execute — PRIVATE. Reachable ONLY via ExecutionGatewayT::submit(), which
    // applies the mode gate + kill-switch + exchange/exposure filters first.
    // A direct strategy call (executor.execute(...)) will NOT compile.
    // -----------------------------------------------------------------------
    // execute — called by the ExecutionGateway on every entry/exit
    //
    // symbol:   lowercase e.g. "btcusdt" — converted to uppercase internally
    // is_buy:   true=BUY, false=SELL
    // qty:      base asset quantity
    // price:    signal price (for logging only — market orders use exchange price)
    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------
    // execute — market order entry/exit, returns OrderResult with fill details
    // -----------------------------------------------------------------------
    // Phase-2 review fix (item 7): accept a DETERMINISTIC client id from the
    // gateway so a retry after an ambiguous send reuses the same id (Binance
    // dedups newClientOrderId) instead of minting a new one and double-buying.
    // Empty => fall back to the legacy timestamp id (unit tests / direct calls).
    OrderResult execute(const std::string& symbol,
                        bool is_buy,
                        double qty,
                        double price,
                        const std::string& client_id = "") {
        OrderResult r;
        if (!rest_.is_ready()) {
            std::fprintf(stderr, "[EXECUTOR] Not ready — order dropped: %s %s %.8f\n",
                         symbol.c_str(), is_buy ? "BUY" : "SELL", qty);
            return r;
        }

        std::string sym_upper = symbol;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);

        std::string cid = client_id;
        if (cid.empty()) {
            auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            cid = sym_upper.substr(0, 3)
                + (is_buy ? "B" : "S")
                + std::to_string(now_us);
        }
        if (cid.size() > 36) cid = cid.substr(cid.size() - 36);

        std::printf("[EXECUTOR] %s %s %.8f @ %.4f signal_px\n",
                    is_buy ? "BUY" : "SELL", sym_upper.c_str(), qty, price);
        std::fflush(stdout);

        r = rest_.place_order(sym_upper, is_buy, qty, cid);
        r.ok = r.ok || r.shadow;  // shadow orders are always "ok"

        if (!r.ok) {
            std::fprintf(stderr, "[EXECUTOR] Order failed: %s\n", r.error.c_str());
            errors_.fetch_add(1, std::memory_order_relaxed);
            return r;
        }

        if (r.shadow) {
            r.executed_qty = qty;
            r.avg_price    = price;
            r.status       = "FILLED";
            fills_.fetch_add(1, std::memory_order_relaxed);
            return r;
        }

        std::printf("[EXECUTOR] FILLED %s %s | id=%ld status=%s qty=%.8f avg_px=%.4f\n",
                    sym_upper.c_str(), is_buy ? "BUY" : "SELL",
                    r.order_id, r.status.c_str(), r.executed_qty, r.avg_price);
        std::fflush(stdout);
        fills_.fetch_add(1, std::memory_order_relaxed);
        return r;
    }

public:
    // -----------------------------------------------------------------------
    // submit_limit_maker — post a maker limit order, returns client_id
    // -----------------------------------------------------------------------
    OrderResult submit_limit_maker(const std::string& symbol,
                                   bool is_buy,
                                   double qty,
                                   double limit_price) {
        OrderResult r;
        if (!rest_.is_ready()) return r;

        std::string sym_upper = symbol;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);

        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string cid = sym_upper.substr(0, 3) + "M"
                        + std::to_string(now_us);
        if (cid.size() > 36) cid = cid.substr(cid.size() - 36);

        if (rest_.is_shadow()) {
            // Shadow: pretend order posted successfully
            r.ok        = true;
            r.shadow    = true;
            r.client_id = cid;
            r.status    = "NEW";
            return r;
        }

        r = rest_.place_limit_maker(sym_upper, is_buy, qty, limit_price, cid);
        if (!r.ok) {
            std::fprintf(stderr, "[EXECUTOR] Limit maker failed: %s\n", r.error.c_str());
            errors_.fetch_add(1, std::memory_order_relaxed);
        }
        return r;
    }

    // -----------------------------------------------------------------------
    // query_order — poll exchange for fill status of a working order
    // -----------------------------------------------------------------------
    OrderResult query_order(const std::string& symbol,
                            const std::string& client_id) {
        OrderResult r;
        if (!rest_.is_ready() || client_id.empty()) return r;
        if (rest_.is_shadow()) {
            // Shadow: not needed, manage_pending uses simulated price crossing
            return r;
        }
        std::string sym_upper = symbol;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);
        return rest_.query_order(sym_upper, client_id);
    }

    // -----------------------------------------------------------------------
    // cancel_working_order — cancel a live limit order
    // -----------------------------------------------------------------------
    bool cancel_working_order(const std::string& symbol,
                              const std::string& client_id,
                              double /*limit_price*/,
                              const char* reason) {
        if (!rest_.is_ready() || client_id.empty()) return false;
        if (rest_.is_shadow()) return true;  // shadow: always succeeds
        std::string sym_upper = symbol;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);
        bool ok = rest_.cancel_order(sym_upper, client_id);
        if (ok) {
            std::printf("[EXECUTOR] Cancelled %s client_id=%s reason=%s\n",
                        sym_upper.c_str(), client_id.c_str(), reason);
            std::fflush(stdout);
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // record_shadow_fill — log a simulated fill in shadow mode
    // -----------------------------------------------------------------------
    void record_shadow_fill(const std::string& symbol,
                            bool is_buy,
                            double qty,
                            double fill_price,
                            const std::string& client_id) {
        std::printf("[SHADOW-FILL] %s %s %.8f @ %.4f cid=%s\n",
                    symbol.c_str(), is_buy ? "BUY" : "SELL",
                    qty, fill_price,
                    client_id.empty() ? "-" : client_id.c_str());
        std::fflush(stdout);
        fills_.fetch_add(1, std::memory_order_relaxed);
    }

    // Phase-2: public pass-throughs to the (public, unsigned) Binance endpoints
    // used to populate exchange filters + clock sync. Safe in shadow.
    int64_t     server_time()                              { return rest_.get_server_time(); }
    std::string exchange_info(const std::string& s = "")   { return rest_.fetch_exchange_info(s); }
    // Pre-boot holdings seed (native-stop residual, 2026-07-24): signed reads used
    // once at LIVE boot to reconstruct + protect a position held before this process.
    std::map<std::string, double> free_balances(const std::vector<std::string>& assets) { return rest_.get_free_balances(assets); }
    double      last_price(const std::string& s)           { return rest_.get_price(s); }
    double      reconstruct_entry(const std::string& s, double q) { return rest_.my_trades_avg_entry(s, q); }

    bool  is_shadow() const { return rest_.is_shadow(); }
    bool  is_ready()  const { return rest_.is_ready();  }
    int   fills()     const { return fills_.load();     }
    int   errors()    const { return errors_.load();    }

    // -----------------------------------------------------------------------
    // place_protective_stop — post a RESTING broker-side SELL stop for a held
    // LONG (crypto twin of Omega's native STP-on-fill). Called by the
    // ExecutionGateway on an opening fill; NOT a strategy-order path (protection,
    // like emergency_flatten — it does not route through the entry gateway).
    //
    // stop_pct is the % below entry the stop triggers. limit_slip_pct extends the
    // limit price BELOW the trigger so the resting limit fills through a fast drop.
    // B5 fix (2026-07-24 audit): default widened 0.5% -> 5%. This is a DISASTER stop
    // (15% trigger) — fill CERTAINTY beats price. A 0.5% band left the limit resting
    // unfilled on a fast gap-through (position naked below the stop); a 5% band fills
    // through almost any realistic gap while still bounding the worst-case fill
    // (~trigger-5%). (Binance spot STOP_LOSS market-on-trigger would be ideal but is
    // pair-restricted; the wide-band STOP_LOSS_LIMIT is the portable equivalent.)
    // Returns the OrderResult (r.client_id / r.order_id identify the resting order
    // for a later cancel-on-flat).
    // -----------------------------------------------------------------------
    OrderResult place_protective_stop(const std::string& symbol,
                                      double qty,
                                      double entry_px,
                                      double stop_pct,
                                      double limit_slip_pct = 5.0) {
        OrderResult r;
        if (!rest_.is_ready() || qty <= 0.0 || entry_px <= 0.0 || stop_pct <= 0.0) return r;
        std::string sym_upper = symbol;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);

        double stop_price  = entry_px * (1.0 - stop_pct / 100.0);
        double limit_price = stop_price * (1.0 - limit_slip_pct / 100.0);
        if (stop_price <= 0.0 || limit_price <= 0.0) return r;

        // Snap BOTH legs of the STOP_LOSS_LIMIT to the symbol's PRICE_FILTER tick,
        // rounding DOWN. An off-tick price is a -1013 PRICE_FILTER reject that loses
        // the protective order. Rounding down keeps the SELL stop protective and
        // keeps limit < stop (limit is already below stop, and floor only lowers).
        if (filters_) {
            stop_price  = filters_->snap_price_to_tick(sym_upper, stop_price);
            limit_price = filters_->snap_price_to_tick(sym_upper, limit_price);
            if (stop_price <= 0.0 || limit_price <= 0.0) return r;
        }

        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string cid = sym_upper.substr(0, 3) + "STP" + std::to_string(now_us);
        if (cid.size() > 36) cid = cid.substr(cid.size() - 36);

        r = rest_.place_stop_loss(sym_upper, qty, stop_price, limit_price, cid);
        if (!r.ok) {
            std::fprintf(stderr, "[EXECUTOR] protective stop failed %s: %s\n",
                         sym_upper.c_str(), r.error.c_str());
            errors_.fetch_add(1, std::memory_order_relaxed);
        }
        return r;
    }

    // cancel_protective_stop — cancel a resting protective stop (cancel-on-flat,
    // mirror of positionEnd). Prefer the client_id; falls back to order_id.
    bool cancel_protective_stop(const std::string& symbol,
                                const std::string& client_id,
                                long order_id = 0) {
        if (!rest_.is_ready()) return false;
        if (rest_.is_shadow()) return true;   // shadow: no real resting order
        std::string sym_upper = symbol;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);
        bool ok = !client_id.empty() ? rest_.cancel_order(sym_upper, client_id)
                                     : (order_id > 0 && rest_.cancel_order(sym_upper, order_id));
        if (ok) {
            std::printf("[EXECUTOR] protective stop cancelled %s cid=%s (position flat)\n",
                        sym_upper.c_str(), client_id.empty() ? "-" : client_id.c_str());
            std::fflush(stdout);
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // emergency_flatten — cancel all open orders for a symbol then market sell qty.
    // Called by emergency kill button. Works in both shadow and live mode.
    // -----------------------------------------------------------------------
    bool emergency_flatten(const std::string& symbol_lower, double qty) {
        std::string sym_upper = symbol_lower;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);
        std::printf("[EMERGENCY-KILL] Flattening %s qty=%.8f\n", sym_upper.c_str(), qty);
        std::fflush(stdout);
        rest_.cancel_all_open_orders(sym_upper);
        if (qty > 0.0) {
            auto r = rest_.market_sell(sym_upper, qty);
            return r.ok;
        }
        return true;
    }

    // Wire the shared exchangeInfo filter cache so protective-stop prices are
    // snapped to PRICE_FILTER tickSize before POST (avoids -1013). Optional: if
    // unset, prices are formatted raw (prior behaviour) and a -1013 reject stays
    // the loud backstop. Same &g_filters instance the gateway uses.
    void set_filters(const ExchangeFilters* f) { filters_ = f; }

private:
    BinanceREST             rest_;
    const ExchangeFilters*  filters_ = nullptr;
    std::atomic<int>        fills_{0};
    std::atomic<int>        errors_{0};
};

} // namespace chimera
