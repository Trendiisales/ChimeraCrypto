#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <vector>
#include <sstream>

namespace chimera {

enum class TradeBlockReason
{
    NONE = 0,
    LATENCY,
    VOLATILITY,
    SPREAD,
    VELOCITY,
    LOSS_STREAK,
    NOT_TOP_RANKED,
    SIGNAL_FALSE
};

struct RejectionCounters
{
    std::atomic<uint64_t> evaluations{0};
    std::atomic<uint64_t> blocks{0};

    std::atomic<uint64_t> latency{0};
    std::atomic<uint64_t> volatility{0};
    std::atomic<uint64_t> spread{0};
    std::atomic<uint64_t> velocity{0};
    std::atomic<uint64_t> loss_streak{0};
    std::atomic<uint64_t> not_ranked{0};
    std::atomic<uint64_t> signal_false{0};
};

class RejectionTelemetryAsync
{
public:
    void recordEvaluation(const std::string& symbol)
    {
        counters(symbol).evaluations.fetch_add(1, std::memory_order_relaxed);
    }

    void recordBlock(const std::string& symbol, TradeBlockReason reason)
    {
        auto& c = counters(symbol);
        c.blocks.fetch_add(1, std::memory_order_relaxed);

        switch (reason)
        {
            case TradeBlockReason::LATENCY:
                c.latency.fetch_add(1, std::memory_order_relaxed); break;
            case TradeBlockReason::VOLATILITY:
                c.volatility.fetch_add(1, std::memory_order_relaxed); break;
            case TradeBlockReason::SPREAD:
                c.spread.fetch_add(1, std::memory_order_relaxed); break;
            case TradeBlockReason::VELOCITY:
                c.velocity.fetch_add(1, std::memory_order_relaxed); break;
            case TradeBlockReason::LOSS_STREAK:
                c.loss_streak.fetch_add(1, std::memory_order_relaxed); break;
            case TradeBlockReason::NOT_TOP_RANKED:
                c.not_ranked.fetch_add(1, std::memory_order_relaxed); break;
            case TradeBlockReason::SIGNAL_FALSE:
                c.signal_false.fetch_add(1, std::memory_order_relaxed); break;
            default:
                break;
        }
    }

    std::string build_json_snapshot() const
    {
        std::ostringstream oss;
        oss << "{";

        bool first_symbol = true;

        for (const auto& pair : symbol_stats_)
        {
            if (!first_symbol) oss << ",";
            first_symbol = false;

            const std::string& symbol = pair.first;
            const RejectionCounters& c = pair.second;

            uint64_t eval = c.evaluations.load(std::memory_order_relaxed);
            uint64_t blk  = c.blocks.load(std::memory_order_relaxed);

            double block_rate = 0.0;
            if (eval > 0)
                block_rate = (double)blk / (double)eval;

            oss << "\"" << symbol << "\":{"
                << "\"evaluations\":" << eval << ","
                << "\"blocks\":" << blk << ","
                << "\"block_rate\":" << block_rate << ","
                << "\"latency\":" << c.latency.load(std::memory_order_relaxed) << ","
                << "\"volatility\":" << c.volatility.load(std::memory_order_relaxed) << ","
                << "\"spread\":" << c.spread.load(std::memory_order_relaxed) << ","
                << "\"velocity\":" << c.velocity.load(std::memory_order_relaxed) << ","
                << "\"loss_streak\":" << c.loss_streak.load(std::memory_order_relaxed) << ","
                << "\"not_ranked\":" << c.not_ranked.load(std::memory_order_relaxed) << ","
                << "\"signal_false\":" << c.signal_false.load(std::memory_order_relaxed)
                << "}";
        }

        oss << "}";
        return oss.str();
    }

private:
    RejectionCounters& counters(const std::string& symbol)
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        return symbol_stats_[symbol];
    }

    mutable std::mutex map_mutex_;
    std::unordered_map<std::string, RejectionCounters> symbol_stats_;
};

}
