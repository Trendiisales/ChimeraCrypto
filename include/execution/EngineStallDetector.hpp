#pragma once
#include <cstdint>
#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

namespace chimera {

struct EngineStallDetector {
    std::atomic<int64_t> last_ws_receive_us{0};
    std::atomic<int64_t> last_eval_start_us{0};
    std::atomic<int64_t> last_eval_end_us{0};
    
    std::atomic<int64_t> max_event_loop_delay_us{0};
    std::atomic<int64_t> max_eval_duration_us{0};
    std::atomic<int64_t> max_total_delay_us{0};
    
    std::atomic<uint64_t> stall_events{0};
    std::atomic<uint64_t> samples{0};
    
    void on_ws_receive() {
        last_ws_receive_us.store(now_us(), std::memory_order_relaxed);
    }
    
    void on_eval_start() {
        int64_t now = now_us();
        last_eval_start_us.store(now, std::memory_order_relaxed);
        
        int64_t ws_recv = last_ws_receive_us.load(std::memory_order_relaxed);
        if (ws_recv > 0) {
            int64_t delay = now - ws_recv;
            
            int64_t old_max = max_event_loop_delay_us.load(std::memory_order_relaxed);
            while (delay > old_max && 
                   !max_event_loop_delay_us.compare_exchange_weak(old_max, delay,
                                                                   std::memory_order_relaxed));
            
            if (delay > 10000) {
                stall_events.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    
    void on_eval_end() {
        int64_t now = now_us();
        last_eval_end_us.store(now, std::memory_order_relaxed);
        
        int64_t start = last_eval_start_us.load(std::memory_order_relaxed);
        if (start > 0) {
            int64_t duration = now - start;
            
            int64_t old_max = max_eval_duration_us.load(std::memory_order_relaxed);
            while (duration > old_max &&
                   !max_eval_duration_us.compare_exchange_weak(old_max, duration,
                                                                std::memory_order_relaxed));
        }
        
        int64_t ws_recv = last_ws_receive_us.load(std::memory_order_relaxed);
        if (ws_recv > 0) {
            int64_t total = now - ws_recv;
            
            int64_t old_max = max_total_delay_us.load(std::memory_order_relaxed);
            while (total > old_max &&
                   !max_total_delay_us.compare_exchange_weak(old_max, total,
                                                              std::memory_order_relaxed));
        }
        
        samples.fetch_add(1, std::memory_order_relaxed);
    }
    
    std::string generate_report() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        
        uint64_t sample_count = samples.load(std::memory_order_relaxed);
        uint64_t stalls = stall_events.load(std::memory_order_relaxed);
        
        double stall_rate = sample_count > 0 ? (double)stalls / sample_count * 100.0 : 0.0;
        
        oss << "\n╔══════════════════════════════════════════════════════════════════════╗\n";
        oss << "║              ENGINE STALL DIAGNOSTIC REPORT                          ║\n";
        oss << "╠══════════════════════════════════════════════════════════════════════╣\n";
        
        oss << "║ Samples: " << sample_count << " | "
            << "Stall Events (>10ms): " << stalls << " ("
            << stall_rate << "%)\n";
        oss << "╟──────────────────────────────────────────────────────────────────────╢\n";
        
        double event_loop = max_event_loop_delay_us.load(std::memory_order_relaxed) / 1000.0;
        double eval_dur = max_eval_duration_us.load(std::memory_order_relaxed) / 1000.0;
        double total = max_total_delay_us.load(std::memory_order_relaxed) / 1000.0;
        
        oss << "║ Max Event Loop Delay:  " << std::setw(8) << event_loop << " ms\n";
        oss << "║ Max Eval Duration:     " << std::setw(8) << eval_dur << " ms\n";
        oss << "║ Max Total Delay:       " << std::setw(8) << total << " ms\n";
        oss << "╟──────────────────────────────────────────────────────────────────────╢\n";
        
        oss << "║ DIAGNOSIS:\n";
        
        if (event_loop > 30.0) {
            oss << "║ ⚠ CRITICAL: Event loop stalling >30ms\n";
            oss << "║   Likely causes:\n";
            oss << "║   • Telemetry broadcast blocking\n";
            oss << "║   • Logging I/O burst\n";
            oss << "║   • WS handler single-threaded\n";
            oss << "║   • JSON serialization spike\n";
        } else if (event_loop > 10.0) {
            oss << "║ ⚠ WARNING: Event loop delay >10ms detected\n";
            oss << "║   Monitor for recurring pattern\n";
        } else {
            oss << "║ ✓ Event loop healthy (<10ms)\n";
        }
        
        if (eval_dur > 5.0) {
            oss << "║ ⚠ WARNING: Evaluation taking >" << eval_dur << "ms\n";
            oss << "║   • Check alpha calculation complexity\n";
            oss << "║   • Review governor evaluation path\n";
        }
        
        if (stall_rate > 5.0) {
            oss << "║ ⚠ HIGH STALL RATE: " << stall_rate << "%\n";
            oss << "║   • Review profiling data\n";
            oss << "║   • Consider async telemetry\n";
        }
        
        oss << "╚══════════════════════════════════════════════════════════════════════╝\n";
        
        return oss.str();
    }
    
    void reset() {
        max_event_loop_delay_us.store(0, std::memory_order_relaxed);
        max_eval_duration_us.store(0, std::memory_order_relaxed);
        max_total_delay_us.store(0, std::memory_order_relaxed);
        stall_events.store(0, std::memory_order_relaxed);
    }

private:
    int64_t now_us() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
};

}
