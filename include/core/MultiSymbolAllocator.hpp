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

    int loss_streak = 0;
    std::chrono::steady_clock::time_point last_disable_time;
};

struct AllocatorConfig
{
    double min_volatility = 0.0015;      // 0.15%
    double max_spread_bps = 4.0;
    double max_latency_ms = 25.0;
    double min_ticks_per_sec = 20.0;

    int max_loss_streak = 2;
    int disable_seconds_after_loss = 600;

    int max_active_symbols = 2;          // only trade top N
};

class MultiSymbolAllocator
{
public:
    explicit MultiSymbolAllocator(const AllocatorConfig& cfg)
        : config_(cfg) {}

    std::vector<std::string> selectActiveSymbols(
        const std::vector<SymbolSnapshot>& symbols)
    {
        struct Ranked
        {
            std::string symbol;
            double score;
        };

        std::vector<Ranked> ranked;

        for (const auto& s : symbols)
        {
            if (!eligible(s))
                continue;

            double score = computeScore(s);
            ranked.push_back({s.symbol, score});
        }

        std::sort(ranked.begin(), ranked.end(),
                  [](const Ranked& a, const Ranked& b)
                  {
                      return a.score > b.score;
                  });

        std::vector<std::string> active;

        for (int i = 0;
             i < config_.max_active_symbols && i < (int)ranked.size();
             ++i)
        {
            active.push_back(ranked[i].symbol);
        }

        return active;
    }

    bool eligible(const SymbolSnapshot& s) const
    {
        if (!latencyPass(s)) return false;
        if (!spreadPass(s)) return false;
        if (!velocityPass(s)) return false;
        if (!volatilityPass(s)) return false;
        if (!lossStreakPass(s)) return false;

        return true;
    }

private:
    AllocatorConfig config_;

    double computeMove(const SymbolSnapshot& s) const
    {
        if (s.ref_price <= 0.0) return 0.0;
        return std::fabs(s.last_price - s.ref_price) / s.ref_price;
    }

    double computeScore(const SymbolSnapshot& s) const
    {
        double move = computeMove(s);
        double velocity = s.ticks_per_sec;
        double spread_quality = 1.0 / std::max(0.0001, s.spread_bps);

        double score =
            (move * 1000.0) +          // volatility weight
            (velocity * 0.1) +         // participation weight
            (spread_quality * 10.0);   // liquidity weight

        return score;
    }

    bool latencyPass(const SymbolSnapshot& s) const
    {
        return s.lat_p95_ms <= config_.max_latency_ms;
    }

    bool spreadPass(const SymbolSnapshot& s) const
    {
        return s.spread_bps <= config_.max_spread_bps;
    }

    bool velocityPass(const SymbolSnapshot& s) const
    {
        return s.ticks_per_sec >= config_.min_ticks_per_sec;
    }

    bool volatilityPass(const SymbolSnapshot& s) const
    {
        return computeMove(s) >= config_.min_volatility;
    }

    bool lossStreakPass(const SymbolSnapshot& s) const
    {
        if (s.loss_streak < config_.max_loss_streak)
            return true;

        auto now = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(
                now - s.last_disable_time).count();

        return elapsed >= config_.disable_seconds_after_loss;
    }
};

}
