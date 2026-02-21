#pragma once
#include <array>
#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>

namespace chimera {

enum LatencyBandEnum {
    BAND_ULTRA_FAST = 0,  // <10ms
    BAND_FAST = 1,        // 10-25ms
    BAND_MEDIUM = 2,      // 25-50ms
    BAND_SLOW = 3,        // 50ms+
    BAND_COUNT = 4
};

struct BandStats {
    std::atomic<int> trade_count{0};
    std::atomic<double> total_pnl_bps{0.0};
    std::atomic<double> total_slippage_bps{0.0};
    std::atomic<int> winning_trades{0};
    
    void record(double pnl_bps, double slippage_bps) {
        trade_count.fetch_add(1, std::memory_order_relaxed);
        
        double old_pnl = total_pnl_bps.load(std::memory_order_relaxed);
        while (!total_pnl_bps.compare_exchange_weak(old_pnl, old_pnl + pnl_bps, 
                                                     std::memory_order_relaxed));
        
        double old_slip = total_slippage_bps.load(std::memory_order_relaxed);
        while (!total_slippage_bps.compare_exchange_weak(old_slip, old_slip + slippage_bps,
                                                          std::memory_order_relaxed));
        
        if (pnl_bps > 0.0) {
            winning_trades.fetch_add(1, std::memory_order_relaxed);
        }
    }
    
    double avg_pnl() const {
        int count = trade_count.load(std::memory_order_relaxed);
        if (count == 0) return 0.0;
        return total_pnl_bps.load(std::memory_order_relaxed) / count;
    }
    
    double avg_slippage() const {
        int count = trade_count.load(std::memory_order_relaxed);
        if (count == 0) return 0.0;
        return total_slippage_bps.load(std::memory_order_relaxed) / count;
    }
    
    double win_rate() const {
        int count = trade_count.load(std::memory_order_relaxed);
        if (count == 0) return 0.0;
        return (double)winning_trades.load(std::memory_order_relaxed) / count;
    }
    
    bool is_profitable() const {
        int count = trade_count.load(std::memory_order_relaxed);
        return count >= 10 && avg_pnl() > 0.0;
    }
};

class PnLByLatencyBand {
public:
    PnLByLatencyBand() = default;
    
    void record_trade(const std::string& symbol, double latency_ms, 
                     double pnl_bps, double slippage_bps) {
        int band_idx = classify_latency(latency_ms);
        auto& band = get_band(symbol, band_idx);
        band.record(pnl_bps, slippage_bps);
        
        global_bands_[band_idx].record(pnl_bps, slippage_bps);
    }
    
    std::string generate_report() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        
        oss << "\n╔══════════════════════════════════════════════════════════════════════╗\n";
        oss << "║              PnL BY LATENCY BAND ANALYSIS                            ║\n";
        oss << "╠══════════════════════════════════════════════════════════════════════╣\n";
        
        oss << "║ GLOBAL (All Symbols)\n";
        oss << "╟──────────────────────────────────────────────────────────────────────╢\n";
        print_band_stats(oss, "ULTRA (<10ms)", global_bands_[BAND_ULTRA_FAST]);
        print_band_stats(oss, "FAST (10-25ms)", global_bands_[BAND_FAST]);
        print_band_stats(oss, "MEDIUM (25-50ms)", global_bands_[BAND_MEDIUM]);
        print_band_stats(oss, "SLOW (>50ms)", global_bands_[BAND_SLOW]);
        
        oss << "╟──────────────────────────────────────────────────────────────────────╢\n";
        oss << "║ PER-SYMBOL BREAKDOWN\n";
        oss << "╟──────────────────────────────────────────────────────────────────────╢\n";
        
        for (const auto& pair : symbol_bands_) {
            const std::string& symbol = pair.first;
            const auto& bands = pair.second;
            
            oss << "║ " << std::setw(10) << std::left << symbol << "\n";
            print_band_stats(oss, "  <10ms", bands[BAND_ULTRA_FAST]);
            print_band_stats(oss, "  10-25ms", bands[BAND_FAST]);
            print_band_stats(oss, "  25-50ms", bands[BAND_MEDIUM]);
            print_band_stats(oss, "  >50ms", bands[BAND_SLOW]);
        }
        
        oss << "╟──────────────────────────────────────────────────────────────────────╢\n";
        oss << "║ RECOMMENDATIONS\n";
        oss << "╟──────────────────────────────────────────────────────────────────────╢\n";
        
        generate_recommendations(oss);
        
        oss << "╚══════════════════════════════════════════════════════════════════════╝\n";
        
        return oss.str();
    }
    
    std::string build_json() const {
        std::ostringstream oss;
        oss << "{\"global\":{";
        
        oss << "\"ultra_fast\":" << band_to_json(global_bands_[BAND_ULTRA_FAST]) << ",";
        oss << "\"fast\":" << band_to_json(global_bands_[BAND_FAST]) << ",";
        oss << "\"medium\":" << band_to_json(global_bands_[BAND_MEDIUM]) << ",";
        oss << "\"slow\":" << band_to_json(global_bands_[BAND_SLOW]);
        
        oss << "},\"symbols\":{";
        
        bool first = true;
        for (const auto& pair : symbol_bands_) {
            if (!first) oss << ",";
            first = false;
            
            const std::string& symbol = pair.first;
            const auto& bands = pair.second;
            
            oss << "\"" << symbol << "\":{";
            oss << "\"ultra_fast\":" << band_to_json(bands[BAND_ULTRA_FAST]) << ",";
            oss << "\"fast\":" << band_to_json(bands[BAND_FAST]) << ",";
            oss << "\"medium\":" << band_to_json(bands[BAND_MEDIUM]) << ",";
            oss << "\"slow\":" << band_to_json(bands[BAND_SLOW]);
            oss << "}";
        }
        
        oss << "}}";
        return oss.str();
    }

private:
    int classify_latency(double latency_ms) const {
        if (latency_ms < 10.0) return BAND_ULTRA_FAST;
        if (latency_ms < 25.0) return BAND_FAST;
        if (latency_ms < 50.0) return BAND_MEDIUM;
        return BAND_SLOW;
    }
    
    BandStats& get_band(const std::string& symbol, int band_idx) {
        return symbol_bands_[symbol][band_idx];
    }
    
    void print_band_stats(std::ostringstream& oss, const std::string& label, 
                         const BandStats& stats) const {
        int count = stats.trade_count.load(std::memory_order_relaxed);
        
        if (count == 0) {
            oss << "║   " << std::setw(14) << std::left << label 
                << " : NO TRADES\n";
            return;
        }
        
        double avg_pnl = stats.avg_pnl();
        double avg_slip = stats.avg_slippage();
        double win_rate = stats.win_rate();
        bool profitable = stats.is_profitable();
        
        oss << "║   " << std::setw(14) << std::left << label << " : "
            << std::setw(4) << std::right << count << " trades | "
            << "PnL: " << std::setw(7) << avg_pnl << "bp | "
            << "Slip: " << std::setw(5) << avg_slip << "bp | "
            << "Win: " << std::setw(4) << (win_rate * 100.0) << "% | "
            << (profitable ? "✓ PROFIT" : "✗ LOSS") << "\n";
    }
    
    std::string band_to_json(const BandStats& stats) const {
        std::ostringstream oss;
        int count = stats.trade_count.load(std::memory_order_relaxed);
        
        oss << "{";
        oss << "\"trades\":" << count << ",";
        oss << "\"avg_pnl_bps\":" << stats.avg_pnl() << ",";
        oss << "\"avg_slippage_bps\":" << stats.avg_slippage() << ",";
        oss << "\"win_rate\":" << stats.win_rate() << ",";
        oss << "\"profitable\":" << (stats.is_profitable() ? "true" : "false");
        oss << "}";
        
        return oss.str();
    }
    
    void generate_recommendations(std::ostringstream& oss) const {
        const auto& slow = global_bands_[BAND_SLOW];
        const auto& medium = global_bands_[BAND_MEDIUM];
        const auto& fast = global_bands_[BAND_FAST];
        
        int slow_count = slow.trade_count.load(std::memory_order_relaxed);
        int medium_count = medium.trade_count.load(std::memory_order_relaxed);
        
        if (slow_count >= 10) {
            double slow_pnl = slow.avg_pnl();
            if (slow_pnl < 0.0) {
                oss << "║ ⚠ CRITICAL: >50ms trades are UNPROFITABLE (" << slow_pnl << "bp avg)\n";
                oss << "║   → HARD BLOCK all >50ms trades immediately\n";
                oss << "║   → Expected impact: Block " << slow_count << " trades, "
                    << "avoid " << std::abs(slow_pnl * slow_count) << "bp losses\n";
            } else if (slow_pnl < 5.0) {
                oss << "║ ⚠ WARNING: >50ms trades barely profitable (" << slow_pnl << "bp avg)\n";
                oss << "║   → Consider tightening to 40ms if edge <5bp\n";
            }
        }
        
        if (medium_count >= 10) {
            double medium_pnl = medium.avg_pnl();
            if (medium_pnl < 0.0) {
                oss << "║ ⚠ WARNING: 25-50ms trades are UNPROFITABLE (" << medium_pnl << "bp avg)\n";
                oss << "║   → Tighten latency limit to 25ms\n";
            } else if (medium_pnl > 8.0) {
                oss << "║ ✓ GOOD: 25-50ms trades profitable (" << medium_pnl << "bp avg)\n";
                oss << "║   → Current 50ms limit is appropriate\n";
            }
        }
        
        double fast_pnl = fast.avg_pnl();
        if (fast.trade_count.load(std::memory_order_relaxed) >= 20 && fast_pnl > 15.0) {
            oss << "║ ✓ EXCELLENT: 10-25ms trades highly profitable (" << fast_pnl << "bp avg)\n";
            oss << "║   → Core latency band performing well\n";
        }
        
        int total_slow = 0;
        for (const auto& pair : symbol_bands_) {
            int sym_slow = pair.second[BAND_SLOW].trade_count.load(std::memory_order_relaxed);
            total_slow += sym_slow;
        }
        
        if (total_slow > 100) {
            oss << "║ ℹ INFO: " << total_slow << " trades in >50ms band\n";
            oss << "║   → Review if infrastructure shock protection is working\n";
        }
    }
    
    std::array<BandStats, BAND_COUNT> global_bands_;
    std::unordered_map<std::string, std::array<BandStats, BAND_COUNT>> symbol_bands_;
};

}
