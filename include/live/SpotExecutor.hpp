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
#include <string>
#include <atomic>
#include <cstdio>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <vector>

namespace chimera {

class SpotExecutor {
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

    // -----------------------------------------------------------------------
    // execute — called by BalancedEngine on every entry/exit
    //
    // symbol:   lowercase e.g. "btcusdt" — converted to uppercase internally
    // is_buy:   true=BUY, false=SELL
    // qty:      base asset quantity
    // price:    signal price (for logging only — market orders use exchange price)
    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------
    // execute — market order entry/exit, returns OrderResult with fill details
    // -----------------------------------------------------------------------
    OrderResult execute(const std::string& symbol,
                        bool is_buy,
                        double qty,
                        double price) {
        OrderResult r;
        if (!rest_.is_ready()) {
            std::fprintf(stderr, "[EXECUTOR] Not ready — order dropped: %s %s %.8f\n",
                         symbol.c_str(), is_buy ? "BUY" : "SELL", qty);
            return r;
        }

        std::string sym_upper = symbol;
        for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);

        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string cid = sym_upper.substr(0, 3)
                        + (is_buy ? "B" : "S")
                        + std::to_string(now_us);
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

    bool  is_shadow() const { return rest_.is_shadow(); }
    bool  is_ready()  const { return rest_.is_ready();  }
    int   fills()     const { return fills_.load();     }
    int   errors()    const { return errors_.load();    }

private:
    BinanceREST         rest_;
    std::atomic<int>    fills_{0};
    std::atomic<int>    errors_{0};
};

} // namespace chimera
