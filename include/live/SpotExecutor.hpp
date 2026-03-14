#pragma once
#include "live/BinanceREST.hpp"
#include <string>
#include <atomic>
#include <cstdio>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace chimera {

class SpotExecutor {
public:
    bool init(const std::string& credentials_path = "config/binance_credentials.json") {
        std::vector<std::string> candidates;
        candidates.push_back(credentials_path);
        if (credentials_path == "config/binance_credentials.json") {
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

        auto bal = rest_.get_account_balance();
        if (!bal.ok) {
            if (rest_.is_shadow()) {
                std::fprintf(stderr,
                             "[EXECUTOR] Shadow mode: account balance unavailable, continuing with paper trading\n");
            } else {
                std::fprintf(stderr, "[EXECUTOR] Cannot fetch account balance - check API key permissions\n");
                return false;
            }
        }

        std::printf("[EXECUTOR] Ready. shadow=%s\n",
                    rest_.is_shadow() ? "YES (paper trading)" : "NO (LIVE)");
        std::fflush(stdout);
        return true;
    }

    OrderResult execute(const std::string& symbol, bool is_buy, double qty, double price) {
        return execute_market(symbol, is_buy, qty, price);
    }

    OrderResult execute_market(const std::string& symbol,
                               bool is_buy,
                               double qty,
                               double price) {
        OrderResult result;
        if (!rest_.is_ready()) {
            result.error = "not_ready";
            std::fprintf(stderr, "[EXECUTOR] Not ready - order dropped: %s %s %.8f\n",
                         symbol.c_str(), is_buy ? "BUY" : "SELL", qty);
            return result;
        }

        std::string sym_upper = to_upper(symbol);
        std::string cid = make_client_id(sym_upper, is_buy ? "MKTB" : "MKTS");

        std::printf("[EXECUTOR] %s %s %.8f @ %.4f signal_px | type=MARKET\n",
                    is_buy ? "BUY" : "SELL", sym_upper.c_str(), qty, price);
        std::fflush(stdout);

        result = rest_.place_order(sym_upper, is_buy, qty, cid, "MARKET", 0.0);
        if (result.ok && result.executed_qty <= 0.0) {
            result.executed_qty = qty;
        }
        if (result.ok && result.avg_price <= 0.0) {
            result.avg_price = price;
        }
        handle_result(sym_upper, is_buy, qty, price, result, true);
        return result;
    }

    OrderResult submit_limit_maker(const std::string& symbol,
                                   bool is_buy,
                                   double qty,
                                   double limit_price) {
        OrderResult result;
        if (!rest_.is_ready()) {
            result.error = "not_ready";
            std::fprintf(stderr, "[EXECUTOR] Not ready - maker order dropped: %s %s %.8f @ %.8f\n",
                         symbol.c_str(), is_buy ? "BUY" : "SELL", qty, limit_price);
            errors_.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        std::string sym_upper = to_upper(symbol);
        std::string cid = make_client_id(sym_upper, is_buy ? "MKRB" : "MKRS");

        std::printf("[EXECUTOR] %s %s %.8f @ %.8f | type=LIMIT_MAKER\n",
                    is_buy ? "BUY" : "SELL", sym_upper.c_str(), qty, limit_price);
        std::fflush(stdout);

        result = rest_.place_limit_maker(sym_upper, is_buy, qty, limit_price, cid);
        if (!result.ok) {
            std::fprintf(stderr, "[EXECUTOR] LIMIT_MAKER failed: %s\n", result.error.c_str());
            errors_.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        if (!result.shadow && (result.status == "FILLED" || result.status == "PARTIALLY_FILLED")) {
            fills_.fetch_add(1, std::memory_order_relaxed);
        }
        return result;
    }

    OrderResult query_order(const std::string& symbol,
                            const std::string& client_id) {
        OrderResult result;
        if (!rest_.is_ready()) {
            result.error = "not_ready";
            return result;
        }
        if (rest_.is_shadow()) {
            result.shadow = true;
            result.error = "shadow_query_unsupported";
            return result;
        }

        result = rest_.get_order(to_upper(symbol), client_id);
        if (!result.ok) {
            errors_.fetch_add(1, std::memory_order_relaxed);
        }
        return result;
    }

    bool record_shadow_fill(const std::string& symbol,
                            bool is_buy,
                            double qty,
                            double fill_price,
                            const std::string& client_id) {
        if (!rest_.is_shadow()) return false;
        std::string sym_upper = to_upper(symbol);
        if (rest_.shadow_record_fill(sym_upper, is_buy, qty, fill_price, client_id)) {
            fills_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    bool cancel_working_order(const std::string& symbol,
                              const std::string& client_id,
                              double limit_price,
                              const char* reason) {
        std::string sym_upper = to_upper(symbol);
        const bool ok = rest_.is_shadow()
            ? rest_.shadow_cancel(sym_upper, client_id, limit_price, reason)
            : rest_.cancel_order(sym_upper, client_id);
        if (!ok) {
            errors_.fetch_add(1, std::memory_order_relaxed);
            std::fprintf(stderr, "[EXECUTOR] Cancel failed: %s %s\n",
                         sym_upper.c_str(), client_id.c_str());
        }
        return ok;
    }

    bool is_shadow() const { return rest_.is_shadow(); }
    bool is_ready() const { return rest_.is_ready(); }
    int fills() const { return fills_.load(); }
    int errors() const { return errors_.load(); }

private:
    static std::string to_upper(const std::string& symbol) {
        std::string out = symbol;
        for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return out;
    }

    static std::string make_client_id(const std::string& symbol, const std::string& tag) {
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string cid = symbol.substr(0, std::min<size_t>(3, symbol.size())) + tag + std::to_string(now_us);
        if (cid.size() > 36) cid = cid.substr(cid.size() - 36);
        return cid;
    }

    void handle_result(const std::string& sym_upper,
                       bool is_buy,
                       double qty,
                       double price,
                       const OrderResult& result,
                       bool count_shadow_fill) {
        if (!result.ok) {
            std::fprintf(stderr, "[EXECUTOR] Order failed: %s\n", result.error.c_str());
            errors_.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        if (result.shadow) {
            if (count_shadow_fill && result.status == "FILLED") {
                fills_.fetch_add(1, std::memory_order_relaxed);
            }
            return;
        }

        std::printf("[EXECUTOR] %s %s | id=%ld type=%s status=%s qty=%.8f avg_px=%.8f signal_px=%.8f\n",
                    sym_upper.c_str(), is_buy ? "BUY" : "SELL",
                    result.order_id, result.order_type.c_str(), result.status.c_str(), qty,
                    result.avg_price, price);
        std::fflush(stdout);

        if (result.status == "FILLED" || result.status == "PARTIALLY_FILLED") {
            fills_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    BinanceREST rest_;
    std::atomic<int> fills_{0};
    std::atomic<int> errors_{0};
};

} // namespace chimera
