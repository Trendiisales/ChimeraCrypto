#pragma once

#include <string>
#include <unordered_map>
#include <array>
#include <atomic>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace chimera {

// Per-symbol latency profiling
struct LatencyProfile {
    // Atomic counters for lock-free updates
    std::atomic<uint64_t> tick_count{0};
    std::atomic<uint64_t> eval_count{0};
    std::atomic<uint64_t> block_count{0};
    
    // Running statistics (updated in batches)
    double sum_latency = 0.0;
    double sum_latency_sq = 0.0;
    uint64_t sample_count = 0;
    
    // Histogram buckets (lock-free)
    std::atomic<uint64_t> bucket_0_5ms{0};    // 0-5ms
    std::atomic<uint64_t> bucket_5_10ms{0};   // 5-10ms
    std::atomic<uint64_t> bucket_10_15ms{0};  // 10-15ms
    std::atomic<uint64_t> bucket_15_25ms{0};  // 15-25ms
    std::atomic<uint64_t> bucket_25_50ms{0};  // 25-50ms
    std::atomic<uint64_t> bucket_50ms_plus{0}; // 50ms+
    
    void record_tick() {
        tick_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    void record_eval() {
        eval_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    void record_block() {
        block_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    void record_latency_sample(double latency_ms) {
        // Update histogram atomically
        if (latency_ms < 5.0) {
            bucket_0_5ms.fetch_add(1, std::memory_order_relaxed);
        } else if (latency_ms < 10.0) {
            bucket_5_10ms.fetch_add(1, std::memory_order_relaxed);
        } else if (latency_ms < 15.0) {
            bucket_10_15ms.fetch_add(1, std::memory_order_relaxed);
        } else if (latency_ms < 25.0) {
            bucket_15_25ms.fetch_add(1, std::memory_order_relaxed);
        } else if (latency_ms < 50.0) {
            bucket_25_50ms.fetch_add(1, std::memory_order_relaxed);
        } else {
            bucket_50ms_plus.fetch_add(1, std::memory_order_relaxed);
        }
        
        // Update running stats (not atomic - periodic sync needed)
        sum_latency += latency_ms;
        sum_latency_sq += latency_ms * latency_ms;
        sample_count++;
    }
    
    double block_rate() const {
        uint64_t evals = eval_count.load(std::memory_order_relaxed);
        uint64_t blocks = block_count.load(std::memory_order_relaxed);
        return evals > 0 ? (double)blocks / (double)evals : 0.0;
    }
    
    double eval_rate() const {
        uint64_t ticks = tick_count.load(std::memory_order_relaxed);
        uint64_t evals = eval_count.load(std::memory_order_relaxed);
        return ticks > 0 ? (double)evals / (double)ticks : 0.0;
    }
    
    double mean_latency() const {
        return sample_count > 0 ? sum_latency / sample_count : 0.0;
    }
    
    double stddev_latency() const {
        if (sample_count < 2) return 0.0;
        double mean = mean_latency();
        double variance = (sum_latency_sq / sample_count) - (mean * mean);
        return variance > 0.0 ? std::sqrt(variance) : 0.0;
    }
};

class LatencyDiagnostics {
public:
    LatencyDiagnostics() = default;
    
    // Record tick arrival (even if no evaluation)
    void on_tick(const std::string& symbol) {
        get_profile(symbol).record_tick();
    }
    
    // Record trade evaluation attempt
    void on_evaluation(const std::string& symbol) {
        get_profile(symbol).record_eval();
    }
    
    // Record latency block
    void on_latency_block(const std::string& symbol) {
        get_profile(symbol).record_block();
    }
    
    // Record latency sample (microsecond precision)
    void on_latency_sample(const std::string& symbol, double latency_ms) {
        get_profile(symbol).record_latency_sample(latency_ms);
    }
    
    // Generate diagnostic report
    std::string generate_report() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        
        oss << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
        oss << "║                   LATENCY DIAGNOSTICS REPORT                     ║\n";
        oss << "╠═══════════════════════════════════════════════════════════════════╣\n";
        
        // Calculate relative load
        uint64_t total_ticks = 0;
        uint64_t total_evals = 0;
        for (const auto& kv : profiles_) {
            const auto& p = kv.second;
            total_ticks += p.tick_count.load(std::memory_order_relaxed);
            total_evals += p.eval_count.load(std::memory_order_relaxed);
        }
        
        for (const auto& kv : profiles_) {
            const std::string& symbol = kv.first;
            const auto& p = kv.second;
            
            uint64_t ticks = p.tick_count.load(std::memory_order_relaxed);
            uint64_t evals = p.eval_count.load(std::memory_order_relaxed);
            uint64_t blocks = p.block_count.load(std::memory_order_relaxed);
            
            double tick_pct = total_ticks > 0 ? (double)ticks / total_ticks * 100.0 : 0.0;
            double eval_pct = total_evals > 0 ? (double)evals / total_evals * 100.0 : 0.0;
            
            oss << "║ " << std::setw(10) << std::left << symbol << " │ "
                << "Ticks: " << std::setw(7) << ticks << " (" << std::setw(5) << tick_pct << "%) │ "
                << "Evals: " << std::setw(7) << evals << " (" << std::setw(5) << eval_pct << "%)\n";
            
            oss << "║            │ "
                << "Blocks: " << std::setw(7) << blocks << " ("
                << std::setw(5) << (p.block_rate() * 100.0) << "%) │ "
                << "Mean: " << std::setw(6) << p.mean_latency() << "ms "
                << "StdDev: " << std::setw(6) << p.stddev_latency() << "ms\n";
            
            // Histogram
            uint64_t b0 = p.bucket_0_5ms.load(std::memory_order_relaxed);
            uint64_t b1 = p.bucket_5_10ms.load(std::memory_order_relaxed);
            uint64_t b2 = p.bucket_10_15ms.load(std::memory_order_relaxed);
            uint64_t b3 = p.bucket_15_25ms.load(std::memory_order_relaxed);
            uint64_t b4 = p.bucket_25_50ms.load(std::memory_order_relaxed);
            uint64_t b5 = p.bucket_50ms_plus.load(std::memory_order_relaxed);
            
            uint64_t total_samples = b0 + b1 + b2 + b3 + b4 + b5;
            
            if (total_samples > 0) {
                oss << "║            │ Histogram: "
                    << "0-5ms:" << std::setw(5) << (b0 * 100 / total_samples) << "% "
                    << "5-10ms:" << std::setw(5) << (b1 * 100 / total_samples) << "% "
                    << "10-15ms:" << std::setw(5) << (b2 * 100 / total_samples) << "% "
                    << "15-25ms:" << std::setw(5) << (b3 * 100 / total_samples) << "% "
                    << "25-50ms:" << std::setw(5) << (b4 * 100 / total_samples) << "% "
                    << "50ms+:" << std::setw(5) << (b5 * 100 / total_samples) << "%\n";
            }
            
            oss << "╟───────────────────────────────────────────────────────────────────╢\n";
        }
        
        // Analysis section
        oss << "║ ANALYSIS:\n";
        
        // Check for disproportionate blocking
        double max_block_rate = 0.0;
        std::string max_block_symbol;
        for (const auto& kv : profiles_) {
            double rate = kv.second.block_rate();
            if (rate > max_block_rate) {
                max_block_rate = rate;
                max_block_symbol = kv.first;
            }
        }
        
        if (max_block_rate > 0.15) {
            oss << "║ ⚠ " << max_block_symbol << " has disproportionate blocking ("
                << (max_block_rate * 100.0) << "%)\n";
            oss << "║   Possible causes:\n";
            oss << "║   • Heavier orderbook updates (more tick processing)\n";
            oss << "║   • More evaluation calls per tick\n";
            oss << "║   • CPU affinity issues\n";
        }
        
        // Check for tail latency issues
        for (const auto& kv : profiles_) {
            const std::string& symbol = kv.first;
            const auto& p = kv.second;
            
            uint64_t b5 = p.bucket_50ms_plus.load(std::memory_order_relaxed);
            uint64_t total = p.bucket_0_5ms.load(std::memory_order_relaxed) +
                           p.bucket_5_10ms.load(std::memory_order_relaxed) +
                           p.bucket_10_15ms.load(std::memory_order_relaxed) +
                           p.bucket_15_25ms.load(std::memory_order_relaxed) +
                           p.bucket_25_50ms.load(std::memory_order_relaxed) +
                           b5;
            
            if (total > 0 && (double)b5 / total > 0.05) {
                oss << "║ ⚠ " << symbol << " has " << ((double)b5 * 100.0 / total)
                    << "% samples >50ms (tail latency issue)\n";
            }
        }
        
        oss << "╚═══════════════════════════════════════════════════════════════════╝\n";
        
        return oss.str();
    }
    
    // JSON export
    std::string build_json() const {
        std::ostringstream oss;
        oss << "{";
        
        bool first = true;
        for (const auto& kv : profiles_) {
            if (!first) oss << ",";
            first = false;
            
            const std::string& symbol = kv.first;
            const auto& p = kv.second;
            
            uint64_t ticks = p.tick_count.load(std::memory_order_relaxed);
            uint64_t evals = p.eval_count.load(std::memory_order_relaxed);
            uint64_t blocks = p.block_count.load(std::memory_order_relaxed);
            
            oss << "\"" << symbol << "\":{"
                << "\"ticks\":" << ticks << ","
                << "\"evals\":" << evals << ","
                << "\"blocks\":" << blocks << ","
                << "\"block_rate\":" << p.block_rate() << ","
                << "\"eval_rate\":" << p.eval_rate() << ","
                << "\"mean_latency_ms\":" << p.mean_latency() << ","
                << "\"stddev_latency_ms\":" << p.stddev_latency() << ","
                << "\"histogram\":{"
                    << "\"0_5ms\":" << p.bucket_0_5ms.load(std::memory_order_relaxed) << ","
                    << "\"5_10ms\":" << p.bucket_5_10ms.load(std::memory_order_relaxed) << ","
                    << "\"10_15ms\":" << p.bucket_10_15ms.load(std::memory_order_relaxed) << ","
                    << "\"15_25ms\":" << p.bucket_15_25ms.load(std::memory_order_relaxed) << ","
                    << "\"25_50ms\":" << p.bucket_25_50ms.load(std::memory_order_relaxed) << ","
                    << "\"50ms_plus\":" << p.bucket_50ms_plus.load(std::memory_order_relaxed)
                << "}"
                << "}";
        }
        
        oss << "}";
        return oss.str();
    }

private:
    LatencyProfile& get_profile(const std::string& symbol) {
        return profiles_[symbol];
    }
    
    std::unordered_map<std::string, LatencyProfile> profiles_;
};

} // namespace chimera
