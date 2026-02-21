#pragma once
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <chrono>

namespace chimera {

struct SymbolSnapshot
{
    std::string symbol;
    double last_price = 0.0;
    double ref_price = 0.0;
    double spread_bps = 0.0;
    double lat_p95_ms = 0.0;
    double ticks_per_sec = 0.0;
    double edge_ema = 0.0;
    int loss_streak = 0;
    std::chrono::steady_clock::time_point last_disable_time;
};

struct AllocatorConfig
{
    double max_latency_ms = 50.0;
    int max_active_symbols = 2;
};

class MultiSymbolAllocator
{
public:
    explicit MultiSymbolAllocator(const AllocatorConfig& cfg)
        : config_(cfg) {}

    std::vector<std::string> selectActiveSymbols(
        const std::vector<SymbolSnapshot>& symbols)
    {
        struct Ranked {
            std::string symbol;
            double score;
        };

        std::vector<Ranked> ranked;

        for (const auto& s : symbols)
        {
            if (s.lat_p95_ms > config_.max_latency_ms)
                continue;

            double score =
                (s.edge_ema * 10.0) +
                (s.ticks_per_sec * 0.1) -
                (s.spread_bps);

            ranked.push_back({s.symbol, score});
        }

        std::sort(ranked.begin(), ranked.end(),
                  [](const Ranked& a, const Ranked& b)
                  { return a.score > b.score; });

        std::vector<std::string> active;
        for (int i = 0;
             i < config_.max_active_symbols && i < (int)ranked.size();
             ++i)
            active.push_back(ranked[i].symbol);

        return active;
    }

private:
    AllocatorConfig config_;
};

}
