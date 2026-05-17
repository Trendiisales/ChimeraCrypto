#pragma once
// ============================================================================
// MultiSymbolFundingFilter.hpp — Per-symbol funding rate filter (Session 30)
//
// Fetches 8h funding rates for ALL symbols from Binance FAPI.
// When funding < -0.05% (shorts paying longs), the spot-long entry has a
// structural tailwind (carry edge). This information is propagated to engines
// which can then:
//   1. Bypass conservative filters (e.g. lower ADX threshold)
//   2. Flag "high conviction" for position sizing
//   3. Reduce the min-edge-bps threshold
//
// USAGE:
//   chimera::MultiSymbolFundingFilter g_funding_filter;
//   // In startup or periodic thread:
//   g_funding_filter.fetch_all();
//   // In 60s loop:
//   for (auto& slot : g_slots) {
//       bool tailwind = g_funding_filter.has_tailwind(slot.symbol_id);
//       slot.engine->set_funding_tailwind(tailwind);
//   }
//
// DATA SOURCE: Binance FAPI /fapi/v1/premiumIndex (all symbols)
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <array>
#include <mutex>
#include <fstream>
#include <string>
#include <chrono>
#include "../SymbolIndex.hpp"

namespace chimera {

class MultiSymbolFundingFilter {
public:
    // Threshold: funding below this = shorts crowded = carry tailwind for longs
    static constexpr double TAILWIND_THRESHOLD = -0.0005;  // -0.05% per 8h = -5bp

    // Threshold for SUPPRESSION: funding above this = longs extremely crowded = risky
    static constexpr double HEADWIND_THRESHOLD = 0.001;    // +0.1% per 8h = +10bp

    struct SymbolFunding {
        double rate       = 0.0;     // raw rate (e.g. 0.0001 = 1bp)
        bool   tailwind   = false;   // negative funding = carry edge
        bool   headwind   = false;   // very positive funding = crowded longs
        int64_t fetch_ts  = 0;       // when fetched (epoch ms)
    };

    MultiSymbolFundingFilter() = default;

    // Blocking fetch for all symbols — call from detached thread every 8h
    void fetch_all() {
        // Binance FAPI premiumIndex endpoint returns all symbols at once
        int ret = ::system(
            "curl -s 'https://fapi.binance.com/fapi/v1/premiumIndex' "
            "> /tmp/chimera_funding_all.json 2>/dev/null"
        );
        (void)ret;

        std::ifstream f("/tmp/chimera_funding_all.json");
        if (!f.is_open()) {
            std::fprintf(stderr, "[FUNDING-FILTER] Failed to read /tmp/chimera_funding_all.json\n");
            return;
        }

        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        f.close();

        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        int found = 0;
        std::lock_guard<std::mutex> lk(mtx_);

        // Parse each symbol's funding rate from the JSON array
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            // Search for the symbol in perp format (e.g. "BTCUSDT")
            std::string search_sym = std::string(SYM_SHORT[i]) + "USDT";
            // Find in JSON: "symbol":"BTCUSDT"... "lastFundingRate":"0.00010000"
            size_t pos = content.find("\"symbol\":\"" + search_sym + "\"");
            if (pos == std::string::npos) continue;

            // Find lastFundingRate after this position
            size_t rate_pos = content.find("\"lastFundingRate\":\"", pos);
            if (rate_pos == std::string::npos || rate_pos > pos + 500) continue;

            rate_pos += 19; // skip "lastFundingRate":"
            size_t end_pos = content.find('"', rate_pos);
            if (end_pos == std::string::npos) continue;

            std::string rate_str = content.substr(rate_pos, end_pos - rate_pos);
            double rate = 0.0;
            try { rate = std::stod(rate_str); }
            catch (...) { continue; }

            rates_[i].rate     = rate;
            rates_[i].tailwind = (rate < TAILWIND_THRESHOLD);
            rates_[i].headwind = (rate > HEADWIND_THRESHOLD);
            rates_[i].fetch_ts = now_ms;
            found++;
        }

        ready_.store(true, std::memory_order_release);

        std::printf("[FUNDING-FILTER] Fetched %d symbols | ", found);
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            if (rates_[i].fetch_ts == now_ms) {
                const char* flag = rates_[i].tailwind ? " TAILWIND" :
                                   rates_[i].headwind ? " HEADWIND" : "";
                std::printf("%s=%.4f%%%s ", SYM_SHORT[i], rates_[i].rate * 100.0, flag);
            }
        }
        std::printf("\n");
        std::fflush(stdout);
    }

    bool is_ready() const { return ready_.load(std::memory_order_acquire); }

    // Does this symbol have a funding tailwind? (shorts paying longs)
    bool has_tailwind(int symbol_id) const {
        if (!ready_.load(std::memory_order_acquire)) return false;
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return false;
        std::lock_guard<std::mutex> lk(mtx_);
        return rates_[symbol_id].tailwind;
    }

    // Is this symbol facing a funding headwind? (longs paying heavily)
    bool has_headwind(int symbol_id) const {
        if (!ready_.load(std::memory_order_acquire)) return false;
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return false;
        std::lock_guard<std::mutex> lk(mtx_);
        return rates_[symbol_id].headwind;
    }

    // Get raw rate for a symbol
    double get_rate(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return 0.0;
        std::lock_guard<std::mutex> lk(mtx_);
        return rates_[symbol_id].rate;
    }

    // Get full info
    SymbolFunding get_funding(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return {};
        std::lock_guard<std::mutex> lk(mtx_);
        return rates_[symbol_id];
    }

private:
    mutable std::mutex mtx_;
    std::array<SymbolFunding, MAX_SYMBOLS> rates_{};
    std::atomic<bool> ready_{false};
};

} // namespace chimera
