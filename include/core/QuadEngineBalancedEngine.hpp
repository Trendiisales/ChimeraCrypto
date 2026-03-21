#pragma once

#include "core/BalancedEngine.hpp"
#include "core/StructuralEngine.hpp"
#include "core/ConvexShockEngine.hpp"
#include "core/CompressionBreakoutEngine.hpp"
#include "core/RegimeStateAllocator.hpp"
#include "telemetry/SimpleHttpServer.hpp"
#include <sstream>
#include <iomanip>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>
#include <fstream>
#include <ctime>

namespace chimera {

// =============================================================
// QUAD ENGINE BALANCED ENGINE
// Micro + Structural + Convex + Compression
// With Regime State Allocator for dynamic capital scaling
// =============================================================

class QuadEngineBalancedEngine {
public:
    QuadEngineBalancedEngine() : http_server_(8080) {
        load_trades_from_disk();
        write_session_marker();

        // Wire BalancedEngine exit callback  our trade log
        balanced_.set_trade_exit_callback([this](const BalancedEngine::TradeExitData& td) {
            push_trade(td.symbol, td.engine, td.pnl_bp,
                       td.entry_price, td.exit_price,
                       td.mfe_bp, td.mae_bp, td.hold_ms, td.reason);
        });
        // Initialize signal engines
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            structural_[i] = StructuralEngine(sym_full(i));
        
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            convex_[i] = ConvexShockEngine(sym_full(i));
        
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            compression_[i] = CompressionBreakoutEngine(sym_full(i));
        
        // Initialize capital allocators vector
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            allocator_.emplace_back(sym_full(i));
        
        http_server_.set_state_callback([this]() {
            return generate_state_json();
        });
        
        if (!http_server_.start()) {
            std::fprintf(stderr, "[QUAD-ENGINE] Failed to start HTTP server\n");
        }
        
        std::printf("\n");
        std::printf("\n");
        std::printf("         QUAD ENGINE + REGIME ALLOCATOR FRAMEWORK              \n");
        std::printf("\n");
        std::printf(" SIGNAL ENGINES:                                                \n");
        std::printf("    MICRO (BalancedEngine):    10-30bp (15bp min)             \n");
        std::printf("    STRUCTURAL:                30-150bp riders                 \n");
        std::printf("    CONVEX SHOCK:              20bp+ acceleration              \n");
        std::printf("    COMPRESSION BREAKOUT:      Tight range  expansion         \n");
        std::printf("                                                                \n");
        std::printf(" CAPITAL INTELLIGENCE:                                          \n");
        std::printf("    Regime State Allocator:    Dynamic capital scaling        \n");
        std::printf("     - DEAD (0.0x)              Kill all trading                \n");
        std::printf("     - COMPRESSION (0.5x)       Conservative sizing             \n");
        std::printf("     - EXPANSION (1.0x)         Normal sizing                   \n");
        std::printf("     - SHOCK (1.5x)             Aggressive scaling              \n");
        std::printf("                                                                \n");
        std::printf(" Portfolio Cap: 2.0R base  0-3.0R dynamic per symbol          \n");
        std::printf(" GUI: http://154.45.251.118:8080                               \n");
        std::printf("\n");
        std::printf("\n");
        std::fflush(stdout);
    }
    
    ~QuadEngineBalancedEngine() {
        http_server_.stop();
    }
    
    void on_tick(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        last_latency_ms_ = latency_ms;  // raw per-tick age for signal gating
        // Derive scalar price for engines that don't need full tick
        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;

        // 1. Run original BalancedEngine (micro) - passes full tick for real data
        balanced_.on_tick(id, tick, ts, latency_ms);
        // Detect new trade completions and log them
        check_new_trades(id);
        
        // 2. Update market state
        update_market_state(id, price, ts);
        
        auto& ms = market_state_[id];
        
        // 3. Run Regime State Allocator FIRST (capital intelligence layer)
        allocator_[id].evaluate(
            ms.vol_ratio,
            ms.short_vol,
            ms.ema_vol,
            ms.displacement_bp,
            ms.acceleration_bp
        );
        
        // 4. Get dynamic capital cap (base 2.0R * regime multiplier)
        double base_cap = 2.0;
        double dynamic_cap = allocator_[id].allowed_R(base_cap);
        
        // 5. Calculate current usage
        double micro_R = balanced_.get_open_positions() > 0 ? 1.0 : 0.0;
        double structural_R = structural_[id].pos.size_R;
        double convex_R = convex_[id].pos.size_R;
        double compression_R = compression_[id].pos.size_R;
        double used_R = micro_R + structural_R + convex_R + compression_R;
        double available_R = std::max(0.0, dynamic_cap - used_R);
        
        // 6. Run signal engines with dynamic capital allocation + ECONOMIC GATES
        
        // STRUCTURAL GATE: 15bp+ displacement, 1.4+ vol ratio
        bool allow_structural = 
            std::abs(ms.displacement_bp) >= 15.0 &&
            ms.vol_ratio >= 1.4 &&
            allocator_[id].get_state() != VolState::DEAD;
        
        if (allow_structural) {
            structural_[id].evaluate(
                price,
                ms.vol_ratio,
                ms.displacement_bp,
                ms.regime,
                ms.vol_rising,
                ts,
                available_R
            );
        }
        
        // Update available R after structural
        structural_R = structural_[id].pos.size_R;
        used_R = micro_R + structural_R + convex_R + compression_R;
        available_R = std::max(0.0, dynamic_cap - used_R);
        
        // CONVEX GATE: 30bp+ displacement, 15bp+ acceleration, 1.8+ vol ratio
        bool allow_convex =
            std::abs(ms.displacement_bp) >= 30.0 &&
            std::abs(ms.acceleration_bp) >= 15.0 &&
            ms.vol_ratio >= 1.8;
        
        if (allow_convex) {
            convex_[id].evaluate(
                price,
                ms.vol_ratio,
                ms.short_vol,
                ms.ema_vol,
                ms.displacement_bp,
                ms.acceleration_bp,
                ms.regime,
                ts,
                available_R
            );
        }
        
        // Update available R after convex
        convex_R = convex_[id].pos.size_R;
        used_R = micro_R + structural_R + convex_R + compression_R;
        available_R = std::max(0.0, dynamic_cap - used_R);
        
        // COMPRESSION GATE: Allow compression detection always
        // (it has its own 100-tick arming + 15bp breakout logic)
        compression_[id].evaluate(
            price,
            ms.vol_ratio,
            ms.displacement_bp,
            ms.short_vol,
            ms.ema_vol,
            ms.acceleration_bp,
            ms.regime,
            ts,
            available_R
        );
        
        // 7. Enforce directional dominance
        enforce_directional_dominance(id);
    }
    
    // Delegate getters
    std::string get_rejection_stats() const { return balanced_.get_rejection_stats(); }
    std::string get_session_stats_json() const { return balanced_.get_session_stats_json(); }
    double get_total_pnl() const { return balanced_.get_total_pnl(); }
    std::string get_boost_json() const { return balanced_.get_boost_json(); }
    double get_realized_pnl() const { return balanced_.get_realized_pnl(); }
    int get_total_trades() const { return balanced_.get_total_trades(); }
    int get_open_positions() const { return balanced_.get_open_positions(); }
    void set_funding_fetcher(chimera::FundingRateFetcher* f) { balanced_.set_funding_fetcher(f); }
    void set_ngas_engine(chimera::NGASLeadLagEngine* n)      { balanced_.set_ngas_engine(n); }
    void set_funding_signal(chimera::FundingSignalEngine* fs) { balanced_.set_funding_signal(fs); }
    void update_coinbase_btc(double price, int64_t ts_ms)     { balanced_.update_coinbase_btc(price, ts_ms); }
    LiquidationEngine& liq_engine() { return balanced_.liq_engine(); }
    void set_executor(chimera::SpotExecutor* e)              { balanced_.set_executor(e); }
    void set_latency(double ms) { last_latency_ms_ = ms; }
    void set_lat_p95(double ms)  { lat_p95_display_  = ms; }
    
    std::string generate_state_json() {
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        json << "{";

        // Build version — git commit hash injected at compile time via -DBUILD_VERSION
        // Falls back to "dev" if not set (local builds without CI)
#ifndef BUILD_VERSION
#define BUILD_VERSION "dev"
#endif
        json << "\"build_ver\":\"" << BUILD_VERSION << "\",";

        for (int _pi = 0; _pi < MAX_SYMBOLS; ++_pi)
            json << "\"" << sym_full(_pi) << "_price\":" << market_state_[_pi].last_price << ",";
        json << "\"pnl\":" << balanced_.get_total_pnl() << ",";
        json << "\"realized_pnl\":" << balanced_.get_realized_pnl() << ",";
        json << "\"open_positions\":" << balanced_.get_open_positions() << ",";
        json << "\"total_trades\":" << balanced_.get_total_trades() << ",";
        json << "\"latency_p95\":" << lat_p95_display_ << ",";
        json << balanced_.get_boost_json() << ",";
        json << balanced_.get_layer_adapt_json() << ",";
        json << "\"rejections\":" << balanced_.get_rejection_stats() << ",";

        // Full session stats  per-layer wins/losses/tp/sl/trail/timeout
        json << balanced_.get_session_stats_json() << ",";

        // Trade log JSON array
        {
            std::lock_guard<std::mutex> lk(trade_log_mutex_);
            json << "\"trade_log\":[";
            bool first_t = true;
            for (auto& tr : trade_log_) {
                if (!first_t) json << ",";
                json << "{\"t\":\"" << tr.time << "\","
                     << "\"s\":\"" << tr.symbol << "\","
                     << "\"e\":\"" << tr.engine << "\","
                     << "\"p\":"  << std::fixed << std::setprecision(2) << tr.pnl_bp << ","
                     << "\"en\":" << std::setprecision(2) << tr.entry_price << ","
                     << "\"ex\":" << std::setprecision(2) << tr.exit_price << ","
                     << "\"mfe\":" << std::setprecision(2) << tr.mfe_bp << ","
                     << "\"mae\":" << std::setprecision(2) << tr.mae_bp << ","
                     << "\"hold\":" << tr.hold_ms << ","
                     << "\"why\":\"" << tr.reason << "\"}";
                first_t = false;
            }
            json << "],";
        }

        for (int i = 0; i < MAX_SYMBOLS; i++) {
            auto structural_stats = structural_[i].get_stats();
            auto convex_stats = convex_[i].get_stats();
            auto compression_stats = compression_[i].get_stats();
            auto& ms = market_state_[i];
            
            json << "\"" << sym_full(i) << "\":{";
            
            // Regime allocator state
            json << "\"regime_state\":\"" << allocator_[i].get_state_name() << "\",";
            json << "\"regime_multiplier\":" << allocator_[i].get_multiplier() << ",";
            json << "\"dynamic_cap_R\":" << allocator_[i].allowed_R(2.0) << ",";
            
            // Micro  BalancedEngine handles all symbols together, no per-symbol micro stats
            json << "\"micro_active\":" << (ms.micro_active ? "true" : "false") << ",";
            json << "\"micro_total_pnl_bp\":0.0,";
            json << "\"micro_total_trades\":0,";
            
            // Structural
            json << "\"structural_active\":" << (structural_stats.active ? "true" : "false") << ",";
            json << "\"structural_size_R\":" << structural_stats.size_R << ",";
            json << "\"structural_entry_price\":" << structural_stats.entry_price << ",";
            json << "\"structural_mfe_bp\":" << structural_stats.mfe_bp << ",";
            json << "\"structural_win_rate\":" << structural_stats.win_rate << ",";
            json << "\"structural_total_pnl_bp\":" << structural_stats.total_pnl_bp << ",";
            json << "\"structural_total_trades\":" << structural_stats.total_trades << ",";
            
            // Convex
            json << "\"convex_active\":" << (convex_stats.active ? "true" : "false") << ",";
            json << "\"convex_size_R\":" << convex_stats.size_R << ",";
            json << "\"convex_entry_price\":" << convex_stats.entry_price << ",";
            json << "\"convex_mfe_bp\":" << convex_stats.mfe_bp << ",";
            json << "\"convex_win_rate\":" << convex_stats.win_rate << ",";
            json << "\"convex_total_pnl_bp\":" << convex_stats.total_pnl_bp << ",";
            json << "\"convex_total_trades\":" << convex_stats.total_trades << ",";
            
            // Compression
            json << "\"compression_active\":" << (compression_stats.active ? "true" : "false") << ",";
            json << "\"compression_size_R\":" << compression_stats.size_R << ",";
            json << "\"compression_mfe_bp\":" << compression_stats.mfe_bp << ",";
            json << "\"compression_entry_price\":" << compression_stats.entry_price << ",";
            json << "\"compression_win_rate\":" << compression_stats.win_rate << ",";
            json << "\"compression_total_pnl_bp\":" << compression_stats.total_pnl_bp << ",";
            json << "\"compression_total_trades\":" << compression_stats.total_trades << ",";
            json << "\"compression_ticks\":" << compression_stats.compression_ticks << ",";
            
            // Portfolio
            double micro_R = ms.micro_active ? 1.0 : 0.0;
            double portfolio_R = micro_R + structural_stats.size_R + convex_stats.size_R + compression_stats.size_R;
            json << "\"portfolio_R\":"  << portfolio_R << ",";

            // Signal readiness (0.0-1.0) for GUI "close to trading" display
            double struct_disp_pct  = std::min(1.0, std::abs(ms.displacement_bp) / 20.0);
            double struct_vol_pct   = std::min(1.0, std::max(0.0, (ms.vol_ratio - 1.0) / 0.4));
            double struct_build_pct = std::min(1.0, ms.buildup_ticks / 40.0);
            json << "\"structural_readiness\":"  << (struct_disp_pct*0.4 + struct_vol_pct*0.4 + struct_build_pct*0.2) << ",";

            double convex_disp_pct  = std::min(1.0, std::abs(ms.displacement_bp) / 30.0);
            double convex_accel_pct = std::min(1.0, std::abs(ms.acceleration_bp) / 15.0);
            double convex_vol_pct   = std::min(1.0, std::max(0.0, (ms.vol_ratio - 1.0) / 0.8));
            json << "\"convex_readiness\":"      << (convex_disp_pct*0.4 + convex_accel_pct*0.4 + convex_vol_pct*0.2) << ",";

            double comp_pct = std::min(1.0, compression_stats.compression_ticks / 100.0);
            json << "\"compression_readiness\":"  << comp_pct << ",";

            json << "\"vol_ratio\":"        << ms.vol_ratio << ",";
            json << "\"displacement_bp\":"  << ms.displacement_bp << ",";
            json << "\"acceleration_bp\":"  << ms.acceleration_bp << ",";
            json << "\"buildup_ticks\":"     << ms.buildup_ticks;
            
            json << "}";
            if (i < MAX_SYMBOLS - 1) json << ",";
        }
        
        json << "}";
        return json.str();
    }

private:
    BalancedEngine balanced_;
    StructuralEngine structural_[MAX_SYMBOLS];
    ConvexShockEngine convex_[MAX_SYMBOLS];
    CompressionBreakoutEngine compression_[MAX_SYMBOLS];
    std::vector<RegimeStateAllocator> allocator_;  // Use vector instead of array
    SimpleHttpServer http_server_;

    double last_latency_ms_  = 0.0;  // per-tick age for signal gating
    double lat_p95_display_  = 0.0;  // rolling p95 for GUI display only

    // Trade log ring buffer
    struct TradeRecord {
        std::string time;
        std::string symbol;
        std::string engine;
        double pnl_bp;
        double entry_price;
        double exit_price;
        double mfe_bp     = 0.0;   // Max Favorable Excursion
        double mae_bp     = 0.0;   // Max Adverse Excursion
        int64_t hold_ms   = 0;     // Hold duration in ms
        std::string reason;        // Exit reason: TP / SL / TRAIL / TIMEOUT
    };
    std::deque<TradeRecord> trade_log_;
    std::mutex trade_log_mutex_;

    // Previous trade counts + cumulative pnl to compute per-trade pnl delta
    int    prev_structural_trades_[MAX_SYMBOLS]  = {};
    double prev_structural_pnl_[MAX_SYMBOLS]     = {};
    int    prev_convex_trades_[MAX_SYMBOLS]      = {};
    double prev_convex_pnl_[MAX_SYMBOLS]         = {};
    int    prev_compression_trades_[MAX_SYMBOLS] = {};
    double prev_compression_pnl_[MAX_SYMBOLS]    = {};

    static constexpr const char* TRADE_LOG_FILE = "data/trade_log.json";
    std::string last_written_trade_key_;  // dedup guard  prevents double-writes

    void load_trades_from_disk() {
        std::ifstream f(TRADE_LOG_FILE);
        if (!f.is_open()) return;
        std::string line;
        std::lock_guard<std::mutex> lk(trade_log_mutex_);
        while (std::getline(f, line)) {
            if (line.size() < 10) continue;
            TradeRecord r;
            auto ex = [&](const std::string& key) -> std::string {
                auto pos = line.find("\"" + key + "\":\"");
                if (pos == std::string::npos) return "";
                pos += key.size() + 4;
                auto end = line.find('"', pos);
                return end != std::string::npos ? line.substr(pos, end-pos) : "";
            };
            auto exd = [&](const std::string& key) -> double {
                auto pos = line.find("\"" + key + "\":");
                if (pos == std::string::npos) return 0.0;
                pos += key.size() + 3;
                auto end = line.find_first_of(",}", pos);
                try { return std::stod(line.substr(pos, end-pos)); } catch(...) { return 0.0; }
            };
            r.time = ex("t"); r.symbol = ex("s"); r.engine = ex("e"); r.reason = ex("why");
            r.pnl_bp = exd("p"); r.entry_price = exd("en"); r.exit_price = exd("ex");
            r.mfe_bp = exd("mfe"); r.mae_bp = exd("mae"); r.hold_ms = (int64_t)exd("hold");
            if (!r.time.empty() && !r.symbol.empty())
                trade_log_.push_back(r);
        }
        if (trade_log_.size() > 200) trade_log_.resize(200);
        std::printf("[TRADE_LOG] Loaded %zu trades from disk\n", trade_log_.size());
    }

    void save_trade_to_disk(const TradeRecord& r) {
        // Ensure data dir exists
        { int _r = ::system("mkdir -p data"); (void)_r; }

        // DEDUP GUARD  prevent double-writes when two processes run simultaneously
        // or when the callback fires twice for the same trade (e.g. after a restart)
        std::string dedup_key = r.time + "|" + r.symbol + "|" + r.engine + "|" +
                                std::to_string((int)(r.pnl_bp * 100));
        if (dedup_key == last_written_trade_key_) {
            std::printf("[TRADE-LOG-DEDUP] skipped duplicate: %s\n", dedup_key.c_str());
            std::fflush(stdout);
            return;
        }
        last_written_trade_key_ = dedup_key;

        std::ofstream f(TRADE_LOG_FILE, std::ios::app);
        if (!f.is_open()) return;
        f << std::fixed << std::setprecision(2)
          << "{\"t\":\"" << r.time << "\","
          << "\"s\":\"" << r.symbol << "\","
          << "\"e\":\"" << r.engine << "\","
          << "\"p\":"  << r.pnl_bp << ","
          << "\"en\":" << r.entry_price << ","
          << "\"ex\":" << r.exit_price << ","
          << "\"mfe\":" << r.mfe_bp << ","
          << "\"mae\":" << r.mae_bp << ","
          << "\"hold\":" << r.hold_ms << ","
          << "\"why\":\"" << r.reason << "\"}\n";
        f.close();

        // Push to git repo so remote (Claude/GUI) can see live trades
        // Engine runs from build/ so we copy up to repo root data/ then push
        { int _r = ::system(
            "cp data/trade_log.json ../data/trade_log.json 2>/dev/null && "
            "cd .. && "
            "git add data/trade_log.json 2>/dev/null && "
            "git commit -m 'data: live trade' --quiet 2>/dev/null && "
            "git push origin main --quiet 2>/dev/null &"
          ); (void)_r; }
    }

    static std::string now_hms() {
        auto t = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::gmtime(&t));
        return std::string(buf);
    }

    void write_session_marker() {
        { int _r = ::system("mkdir -p data"); (void)_r; }
        std::ofstream f(TRADE_LOG_FILE, std::ios::app);
        if (!f.is_open()) return;
        f << "{\"t\":\"" << now_hms() << "\",\"s\":\"SESSION\",\"e\":\"START\","
          << "\"p\":0,\"en\":0,\"ex\":0,\"mfe\":0,\"mae\":0,\"hold\":0,\"why\":\"START\"}\n";
    }

    void push_trade(const std::string& sym, const std::string& eng,
                    double pnl_bp, double entry_px, double current_px,
                    double mfe_bp = 0.0, double mae_bp = 0.0,
                    int64_t hold_ms = 0, const std::string& reason = "") {
        TradeRecord r;
        r.time        = now_hms();
        r.symbol      = sym;
        r.engine      = eng;
        r.pnl_bp      = pnl_bp;
        r.entry_price = entry_px;
        r.exit_price  = current_px;
        r.mfe_bp      = mfe_bp;
        r.mae_bp      = mae_bp;
        r.hold_ms     = hold_ms;
        r.reason      = reason.empty() ? (pnl_bp >= 0 ? "TP" : "SL") : reason;
        save_trade_to_disk(r);
        // IN-MEMORY DEDUP  prevent disk-loaded records appearing twice when live
        // callback fires for a trade already in memory from load_trades_from_disk()
        std::string mem_key = r.time + "|" + r.symbol + "|" + r.engine + "|" +
                              std::to_string((int)(r.pnl_bp * 100));
        std::lock_guard<std::mutex> lk(trade_log_mutex_);
        for (const auto& t : trade_log_) {
            std::string k = t.time + "|" + t.symbol + "|" + t.engine + "|" +
                            std::to_string((int)(t.pnl_bp * 100));
            if (k == mem_key) {
                std::printf("[TRADE-MEM-DEDUP] skipped in-memory duplicate: %s\n", mem_key.c_str());
                std::fflush(stdout);
                return;
            }
        }
        trade_log_.push_front(r);
        if (trade_log_.size() > 200) trade_log_.pop_back();
    }

    void check_new_trades(int id) {
        // sym_short(i) used directly below
        const double px = market_state_[id].last_price;

        auto ss = structural_[id].get_stats();
        auto cs = convex_[id].get_stats();
        auto xs = compression_[id].get_stats();

        // Structural exit detected
        if (ss.total_trades > prev_structural_trades_[id]) {
            double trade_pnl = ss.total_pnl_bp - prev_structural_pnl_[id];
            push_trade(sym_short(id), "STRUCT", trade_pnl, ss.entry_price, px);
            prev_structural_trades_[id] = ss.total_trades;
            prev_structural_pnl_[id]    = ss.total_pnl_bp;
        }

        // Convex exit detected
        if (cs.total_trades > prev_convex_trades_[id]) {
            double trade_pnl = cs.total_pnl_bp - prev_convex_pnl_[id];
            push_trade(sym_short(id), "CONVEX", trade_pnl, cs.entry_price, px);
            prev_convex_trades_[id] = cs.total_trades;
            prev_convex_pnl_[id]    = cs.total_pnl_bp;
        }

        // Compression exit detected
        if (xs.total_trades > prev_compression_trades_[id]) {
            double trade_pnl = xs.total_pnl_bp - prev_compression_pnl_[id];
            push_trade(sym_short(id), "COMP", trade_pnl, xs.entry_price, px);
            prev_compression_trades_[id] = xs.total_trades;
            prev_compression_pnl_[id]    = xs.total_pnl_bp;
        }
    }
    
    struct SimpleMarketState {
        double last_price = 0.0;
        double vol_ratio = 1.0;
        double short_vol = 0.0;
        double ema_vol = 0.0;
        double displacement_bp = 0.0;
        double acceleration_bp = 0.0;
        int regime = 0;
        bool vol_rising = false;
        
        bool micro_active = false;
        
        std::deque<double> returns;
        std::deque<double> price_deltas;
        int buildup_ticks = 0;

        // Per-symbol anchor for displacement (was incorrectly static/shared)
        double anchor_price = 0.0;
        int tick_counter = 0;
    };
    
    SimpleMarketState market_state_[MAX_SYMBOLS];
    
    void update_market_state(int id, double price, int64_t ts) {
        auto& ms = market_state_[id];
        
        if (ms.last_price > 0) {
            double ret = std::log(price / ms.last_price);
            ms.returns.push_back(ret);
            if (ms.returns.size() > 20) ms.returns.pop_front();
            
            double delta_bp = (price - ms.last_price) / ms.last_price * 10000.0;
            ms.price_deltas.push_back(delta_bp);
            if (ms.price_deltas.size() > 12) ms.price_deltas.pop_front();
            
            if (ms.price_deltas.size() >= 2) {
                ms.acceleration_bp = ms.price_deltas.back() - 
                                    ms.price_deltas[ms.price_deltas.size() - 2];
            }
            
            double sum_sq = 0.0;
            for (double r : ms.returns) sum_sq += r * r;
            ms.short_vol = std::sqrt(sum_sq / ms.returns.size());
            
            if (ms.ema_vol == 0.0) ms.ema_vol = ms.short_vol;
            else ms.ema_vol = 0.95 * ms.ema_vol + 0.05 * ms.short_vol;
            
            ms.vol_ratio = (ms.ema_vol > 0) ? ms.short_vol / ms.ema_vol : 1.0;
            ms.vol_rising = (ms.short_vol > ms.ema_vol * 1.1);
            
            if (ms.vol_ratio > 1.3) {
                ms.regime = 2;
                ms.buildup_ticks++;
            } else if (ms.vol_ratio > 0.8) {
                ms.regime = 1;
                ms.buildup_ticks = 0;
            } else {
                ms.regime = 0;
                ms.buildup_ticks = 0;
            }
            
            if (ms.tick_counter % 20 == 0) ms.anchor_price = price;
            if (ms.anchor_price > 0) {
                ms.displacement_bp = (price - ms.anchor_price) / ms.anchor_price * 10000.0;
            }
        }
        
        ms.last_price = price;
        ms.tick_counter++;
        ms.micro_active = (balanced_.get_open_positions() > 0);
    }
    
    void enforce_directional_dominance(int id) {
        // Structural has priority over convex and compression
        if (structural_[id].pos.active) {
            auto struct_dir = structural_[id].pos.dir;
            
            // Check convex conflict
            if (convex_[id].pos.active) {
                auto convex_dir = convex_[id].pos.dir;
                bool conflict = false;
                if (struct_dir == StructDirection::LONG && convex_dir == ConvexDirection::SHORT) conflict = true;
                if (struct_dir == StructDirection::SHORT && convex_dir == ConvexDirection::LONG) conflict = true;
                
                if (conflict) {
                    std::printf("[DOMINANCE] %s | Convex forced exit - conflicts with Structural\n",
                        convex_[id].symbol.c_str());
                    std::fflush(stdout);
                    convex_[id].reset();
                    convex_[id].cooldown_ticks = 45;
                }
            }
            
            // Check compression conflict
            if (compression_[id].pos.active) {
                auto comp_dir = compression_[id].pos.dir;
                bool conflict = false;
                if (struct_dir == StructDirection::LONG && comp_dir == CompressionDirection::SHORT) conflict = true;
                if (struct_dir == StructDirection::SHORT && comp_dir == CompressionDirection::LONG) conflict = true;
                
                if (conflict) {
                    std::printf("[DOMINANCE] %s | Compression forced exit - conflicts with Structural\n",
                        compression_[id].symbol.c_str());
                    std::fflush(stdout);
                    compression_[id].pos.reset();
                    compression_[id].cooldown_ticks = 45;
                }
            }
        }
    }
};

} // namespace chimera
