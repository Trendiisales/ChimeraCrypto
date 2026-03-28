#pragma once

#include "config/TradingConfig.hpp"

// Build version — written to include/version_generated.hpp by cmake/GenVersion.cmake
// on every make. Lives in include/ so normal quoted #include finds it.
#include "version_generated.hpp"
#ifndef BUILD_VERSION
#  define BUILD_VERSION "dev"
#endif
#include "core/BalancedEngine.hpp"
#include "core/StructuralEngine.hpp"
#include "core/ConvexShockEngine.hpp"
#include "core/CompressionBreakoutEngine.hpp"
#include "core/OrderbookImbalanceEngine.hpp"
#include "core/AggressiveFlowEngine.hpp"
#include "core/PullbackContinuationEngine.hpp"
#include "core/LiqBracketEngine.hpp"
#include "core/BasisMomentumEngine.hpp"
#include "core/FundingWindowEngine.hpp"
#include "live/PerpFeed.hpp"
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
            obi_[i]  = OrderbookImbalanceEngine(sym_full(i));
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            afe_[i]  = AggressiveFlowEngine(sym_full(i));
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            pce_[i]  = PullbackContinuationEngine(sym_full(i));
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            bracket_[i] = LiqBracketEngine(sym_full(i));
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            basis_[i] = BasisMomentumEngine(sym_full(i));
        // FundingWindow: BTC and ETH only
        fund_win_[0] = FundingWindowEngine(sym_full(0));
        fund_win_[1] = FundingWindowEngine(sym_full(1));
        
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

        // Emergency kill command — fired by GUI kill button
        http_server_.set_command_callback([this](const std::string& cmd, const std::string& body) -> std::string {
            if (cmd == "kill_all") {
                std::printf("[QUAD-ENGINE] EMERGENCY KILL ALL received from GUI\n");
                std::fflush(stdout);
                balanced_.emergency_flatten_all();
                return "{\"ok\":true,\"msg\":\"All positions flattened\"}";
            }
            if (cmd == "flatten") {
                // body = {"sym":"BTC"}
                auto pos = body.find("\"sym\":\"");
                if (pos != std::string::npos) {
                    pos += 7;
                    auto end = body.find('"', pos);
                    if (end != std::string::npos) {
                        std::string sym = body.substr(pos, end - pos);
                        std::printf("[QUAD-ENGINE] FLATTEN %s received from GUI\n", sym.c_str());
                        std::fflush(stdout);
                        balanced_.emergency_flatten_symbol(sym);
                        return "{\"ok\":true,\"msg\":\"Flattened " + sym + "\"}";
                    }
                }
                return "{\"ok\":false,\"error\":\"missing sym in body\"}";
            }
            return "{\"ok\":false,\"error\":\"unknown command\"}";
        });

        if (!http_server_.start()) {
            std::fprintf(stderr, "[QUAD-ENGINE] Failed to start HTTP server\n");
        }

        // Restore any positions that were open when the engine last stopped.
        // Must be called after http_server_.start() so the GUI can reflect them.
        balanced_.restore_from_journal();

        // Print accurate startup banner — BUILD_VERSION injected at compile time by CMake
        {
            const char* git_hash = BUILD_VERSION;
            std::printf("\n");
            std::printf("  ╔══════════════════════════════════════════════════════╗\n");
            std::printf("  ║          CHIMERA QUAD ENGINE  build %-8s        ║\n", git_hash);
            std::printf("  ╠══════════════════════════════════════════════════════╣\n");
            std::printf("  ║  ACTIVE STRATEGIES                                   ║\n");
            std::printf("  ║    LIQ CASCADE     TP=150bp trail  SL=20bp  ✓       ║\n");
            std::printf("  ║    VWAP REVERSION  TP=30bp  entry≥25bp  SL=5bp ✓   ║\n");
            std::printf("  ║    LEADLAG         TP=12bp  BTC→ETH/SOL     ✓       ║\n");
            std::printf("  ║    MM PRESSURE     TP=150bp trail  SL=10bp  ✓       ║\n");
            std::printf("  ║    FUNDING CARRY   TP=30bp  2hr hold        ✓       ║\n");
            std::printf("  ║    NGAS LEAD-LAG   TP=35bp  1hr hold        ✓       ║\n");
            std::printf("  ║    VOLSHOCK        TP=25bp  maker entry     ✓       ║\n");
            std::printf("  ║    STAT ARB        TP=20bp  BTC/ETH z>2     ✓       ║\n");
            std::printf("  ║    SESSION MOM     TP=22bp  EU/US opens     ✓       ║\n");
            std::printf("  ║    BASIS MOMENTUM  trail    BTC/ETH only    ✓       ║\n");
            std::printf("  ║    FUND WINDOW     trail    pre-funding     ✓       ║\n");
            std::printf("  ║    LIQ BRACKET     trail    BTC/ETH/SOL     ✓       ║\n");
            std::printf("  ╠══════════════════════════════════════════════════════╣\n");
            std::printf("  ║  SYMBOL SIZING                                       ║\n");
            std::printf("  ║    BTC 1.00x  ETH 0.80x  SOL 0.60x  BNB 0.40x      ║\n");
            std::printf("  ║    AVAX/LINK 0.15x  XRP 0.40x  (BNB-tier liquidity) ║\n");
            std::printf("  ╠══════════════════════════════════════════════════════╣\n");
            std::printf("  ║  COST FLOOR: 15bp maker / 22bp taker                ║\n");
            std::printf("  ║  24/7 TRADING: no kill window — crypto never sleeps  ║\n");
            std::printf("  ║  PYRAMIDING: armed at +30bp, unit2=50%% size        ║\n");
            std::printf("  ║  PERSISTENCE: positions survive restart             ║\n");
            std::printf("  ║  GUI: http://154.45.251.118:8080                    ║\n");
            std::printf("  ╚══════════════════════════════════════════════════════╝\n");
            std::printf("\n");
            std::fflush(stdout);
        }
    }  // end constructor

    ~QuadEngineBalancedEngine() {
        http_server_.stop();
    }
    
    void on_tick(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        last_latency_ms_ = latency_ms;
        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;

        // ALWAYS update market state and run BalancedEngine — GUI needs live prices
        // GUI always has live prices. Trading gates enforced inside each engine.
        balanced_.on_tick(id, tick, ts, latency_ms);
        check_new_trades(id);
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
        
        // STRUCTURAL: DISABLED (Option B — 15bp cost floor, 25bp TP insufficient)
        bool allow_structural = false; // net +10bp TP requires 73% WR, not viable
        
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
        
        // CONVEX: DISABLED (Option B — 40bp TP gives +25bp net, needs 54% WR, insufficient samples)
        bool allow_convex = false;
        
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
        
        // COMPRESSION: DISABLED (Option B — 30bp TP gives +15bp net, needs 67% WR, not viable)
        // compression_[id].evaluate(  // RE-ENABLE when targets raised to 60bp+
        if (false) compression_[id].evaluate(
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

        // 7b. Update flow EMAs for AggressiveFlowEngine
        update_flow_ema(id, tick.agg_buy_volume, tick.agg_sell_volume);

        // Recalculate available_R after structural/convex/compression
        double obi_R = obi_[id].pos_active_ ? obi_[id].pos_size_R_ : 0.0;
        double afe_R = afe_[id].pos_active_ ? afe_[id].pos_size_R_ : 0.0;
        double pce_R = pce_[id].pos_active_ ? pce_[id].pos_size_R_ : 0.0;
        used_R = micro_R + structural_R + convex_R + compression_R + obi_R + afe_R + pce_R;
        available_R = std::max(0.0, dynamic_cap - used_R);

        // 7c. Orderbook Imbalance Engine
        double perp_basis_bp = perp_feed_ && perp_feed_->ready(id) ? perp_feed_->basis_bp(id, price) : 0.0;
        double perp_flow     = perp_feed_ && perp_feed_->ready(id) ? perp_feed_->perp_flow_ratio(id) : 0.0;
        double perp_funding  = perp_feed_ && perp_feed_->ready(id) ? perp_feed_->funding_rate(id)    : 0.0;
        // OBI: DISABLED (Option B — 25bp TP gives +10bp net, needs 69% WR, not viable)
        bool allow_obi = false;
        if (allow_obi) {
            obi_[id].evaluate(
                price,
                tick.book_imbalance,
                tick.spread_bps,
                ms.vol_ratio,
                perp_basis_bp,
                ms.regime,
                ts,
                available_R
            );
            if (obi_[id].pos_active_) micro_engine_trades_in_window_--; // refund if already active
        }

        // AFE: DISABLED (Option B — 30bp TP gives +15bp net, needs 61% WR, marginal)
        bool allow_afe = false;
        if (allow_afe) {
            afe_[id].evaluate(
                price,
                ms.buy_vol_ema,
                ms.sell_vol_ema,
                tick.spread_bps,
                ms.vol_ratio,
                perp_flow,
                ms.regime,
                ts,
                available_R
            );
            if (afe_[id].pos_active_) micro_engine_trades_in_window_--;
        }

        // PCE: DISABLED (Option B — 30bp TP gives +15bp net, needs 61% WR, marginal)
        bool allow_pce = false;
        if (allow_pce) {
            pce_[id].evaluate(
                price,
                ms.displacement_bp,
                ms.acceleration_bp,
                tick.spread_bps,
                ms.vol_ratio,
                perp_funding,
                ms.regime,
                ts,
                available_R
            );
            if (pce_[id].pos_active_) micro_engine_trades_in_window_--;
        }

        // 7f. Liquidation Bracket Engine — BTC/ETH/SOL only (alts have too-thin books for bracket)
        // Capital gate: only if significant room left after other engines
        double bracket_available = std::max(0.0, dynamic_cap - used_R - obi_R - afe_R - pce_R);
        if (bracket_available >= 0.5 && id <= 2) {  // BTC=0, ETH=1, SOL=2 only
            double bracket_liq     = balanced_.liq_engine().get_notional(id);
            double bracket_basis   = perp_feed_ && perp_feed_->ready(id)
                                     ? perp_feed_->basis_bp(id, price) : 0.0;
            bracket_[id].evaluate(
                price,
                ms.vol_ratio,
                bracket_liq,
                bracket_basis,
                ms.regime,
                ts,
                bracket_available
            );
        }

        // 7g. Basis Momentum Engine — BTC and ETH only (depth required for perp->spot lead-lag)
        if (id <= 1 && perp_feed_ && perp_feed_->ready(id)) {
            double basis_available = std::max(0.0, dynamic_cap - used_R
                                              - (basis_[id].pos_active_ ? basis_[id].pos_size_R_ : 0.0));
            basis_[id].evaluate(
                price,
                perp_basis_bp,
                perp_flow,
                ms.vol_ratio,
                ts,
                basis_available
            );
        }

        // 7h. Funding Window Engine — BTC and ETH only, pre-funding 3min window
        if (id <= 1 && perp_feed_ && perp_feed_->ready(id)) {
            double fw_rate  = perp_feed_->funding_rate(id);
            double fw_basis = perp_feed_->basis_bp(id, price);
            double fw_avail = std::max(0.0, dynamic_cap - used_R
                             - (fund_win_[id].pos_active_ ? fund_win_[id].pos_size_R_ : 0.0));
            fund_win_[id].evaluate(price, fw_rate, fw_basis, ts, fw_avail);
        }

        // 8. Enforce directional dominance
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
    void set_perp_feed(chimera::PerpFeed* pf)                 { perp_feed_ = pf; }
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
        // Build version injected at compile time by CMake (BUILD_VERSION macro)
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
            
            // Micro/LIQ — BalancedEngine
            json << "\"micro_active\":" << (ms.micro_active ? "true" : "false") << ",";
            json << "\"micro_total_pnl_bp\":0.0,";
            json << "\"micro_total_trades\":0,";
            // liq_active is same as micro_active — BalancedEngine handles per-symbol in s.pos
            json << "\"liq_active\":" << (ms.micro_active ? "true" : "false") << ",";
            
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

            // OBI
            auto obi_s = obi_[i].get_stats();
            json << "\"obi_active\":" << (obi_s.active ? "true" : "false") << ",";
            json << "\"obi_total_pnl_bp\":" << obi_s.total_pnl_bp << ",";
            json << "\"obi_total_trades\":" << obi_s.total_trades << ",";
            json << "\"obi_win_rate\":" << obi_s.win_rate << ",";

            // AFE
            auto afe_s = afe_[i].get_stats();
            json << "\"afe_active\":" << (afe_s.active ? "true" : "false") << ",";
            json << "\"afe_total_pnl_bp\":" << afe_s.total_pnl_bp << ",";
            json << "\"afe_total_trades\":" << afe_s.total_trades << ",";
            json << "\"afe_win_rate\":" << afe_s.win_rate << ",";

            // PCE
            auto pce_s = pce_[i].get_stats();
            json << "\"pce_active\":" << (pce_s.active ? "true" : "false") << ",";
            json << "\"pce_total_pnl_bp\":" << pce_s.total_pnl_bp << ",";
            json << "\"pce_total_trades\":" << pce_s.total_trades << ",";
            json << "\"pce_win_rate\":" << pce_s.win_rate << ",";

            // Funding Window (BTC/ETH only)
            if (i <= 1) {
                double fw_rate  = (perp_feed_ && perp_feed_->ready(i)) ? perp_feed_->funding_rate(i) : 0.0;
                double fw_basis = (perp_feed_ && perp_feed_->ready(i)) ? perp_feed_->basis_bp(i, ms.last_price) : 0.0;
                auto fw_s = fund_win_[i].get_stats(fw_rate, fw_basis);
                json << "\"fundwin_active\":"      << (fw_s.active ? "true" : "false") << ",";
                json << "\"fundwin_secs_to_next\":" << fw_s.secs_to_next_funding << ",";
                json << "\"fundwin_rate_bp\":"     << (fw_s.current_rate * 10000.0) << ",";
                json << "\"fundwin_total_pnl\":"   << fw_s.total_pnl_bp << ",";
                json << "\"fundwin_trades\":"      << fw_s.total_trades << ",";
                if (fw_s.active) {
                    double fw_move = fw_s.entry_price > 0
                        ? (ms.last_price - fw_s.entry_price) / fw_s.entry_price * 10000.0 : 0.0;
                    json << "\"fundwin_move_bp\":"  << fw_move << ",";
                    json << "\"fundwin_mfe_bp\":"   << fw_s.mfe_bp << ",";
                }
            }

            // Basis Momentum
            auto ba_s = basis_[i].get_stats();
            json << "\"basis_active\":"     << (ba_s.active ? "true" : "false") << ",";
            json << "\"basis_total_pnl_bp\":" << ba_s.total_pnl_bp << ",";
            json << "\"basis_total_trades\":" << ba_s.total_trades << ",";
            json << "\"basis_win_rate\":"    << ba_s.win_rate << ",";
            if (ba_s.active) {
                double ba_move = ba_s.entry_price > 0
                    ? (ms.last_price - ba_s.entry_price) / ba_s.entry_price * 10000.0 : 0.0;
                double ba_trail_dist = ba_s.mfe_bp < 50 ? 20.0 : ba_s.mfe_bp < 100 ? 18.0
                                     : ba_s.mfe_bp < 200 ? 15.0 : 12.0;
                double ba_trail_floor = ba_s.mfe_bp >= 20.0
                    ? ba_s.mfe_bp - ba_trail_dist : -9999.0;
                json << "\"basis_entry\":" << ba_s.entry_price << ",";
                json << "\"basis_move_bp\":" << ba_move << ",";
                json << "\"basis_mfe_bp\":" << ba_s.mfe_bp << ",";
                json << "\"basis_trail_floor\":" << ba_trail_floor << ",";
                json << "\"basis_trail_armed\":" << (ba_s.mfe_bp >= 20.0 ? "true" : "false") << ",";
            }

            // Bracket
            auto bk_s = bracket_[i].get_stats();
            json << "\"bracket_active\":" << (bk_s.active ? "true" : "false") << ",";
            json << "\"bracket_total_pnl_bp\":" << bk_s.total_pnl_bp << ",";
            json << "\"bracket_total_trades\":" << bk_s.total_trades << ",";
            json << "\"bracket_win_rate\":" << bk_s.win_rate << ",";
            json << "\"bracket_range_pct\":" << bk_s.range_pct << ",";
            {
                const char* bk_state_str =
                    bk_s.state == LiqBracketEngine::State::IDLE         ? "IDLE"
                  : bk_s.state == LiqBracketEngine::State::RANGE_BUILD  ? "RANGE_BUILD"
                  : bk_s.state == LiqBracketEngine::State::WAIT_CONFIRM ? "WAIT_CONFIRM"
                  : bk_s.state == LiqBracketEngine::State::ARMED        ? "ARMED"
                  : bk_s.state == LiqBracketEngine::State::IN_POSITION  ? "IN_POSITION"
                  : "COOLDOWN";
                json << "\"bracket_state\":\"" << bk_state_str << "\"," ;
            }
            // Live bracket position data
            if (bk_s.active) {
                double bk_move = bk_s.entry_price > 0
                    ? (ms.last_price - bk_s.entry_price) / bk_s.entry_price * 10000.0 : 0.0;
                double bk_trail_arm = 40.0;
                double bk_trail_dist = bk_move < 50 ? 20.0 : bk_move < 100 ? 18.0
                                     : bk_move < 200 ? 15.0 : bk_move < 300 ? 12.0 : 8.0;
                double bk_trail_floor = bk_s.mfe_bp >= bk_trail_arm
                    ? bk_s.mfe_bp - bk_trail_dist : -9999.0;
                json << "\"bracket_entry\":" << bk_s.entry_price << ",";
                json << "\"bracket_move_bp\":" << bk_move << ",";
                json << "\"bracket_mfe_bp\":" << bk_s.mfe_bp << ",";
                json << "\"bracket_trail_floor\":" << bk_trail_floor << ",";
                json << "\"bracket_trail_armed\":" << (bk_s.mfe_bp >= bk_trail_arm ? "true" : "false") << ",";
            }

            // Perp feed data for GUI display
            double _pbasis  = (perp_feed_ && perp_feed_->ready(i)) ? perp_feed_->basis_bp(i, ms.last_price) : 0.0;
            double _pfund   = (perp_feed_ && perp_feed_->ready(i)) ? perp_feed_->funding_rate(i) : 0.0;
            double _liqnot  = balanced_.liq_engine().get_notional(i);
            json << "\"perp_basis_bp\":"    << _pbasis  << ",";
            json << "\"perp_funding_rate\":" << _pfund   << ",";
            json << "\"liq_notional\":"       << _liqnot  << ",";

            // Portfolio
            double micro_R = ms.micro_active ? 1.0 : 0.0;
            double portfolio_R = micro_R + structural_stats.size_R + convex_stats.size_R + compression_stats.size_R
                                + obi_[i].pos_size_R_ + afe_[i].pos_size_R_ + pce_[i].pos_size_R_
                                + bracket_[i].pos.size_R;
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
            json << "\"buildup_ticks\":" << ms.buildup_ticks << ",";
            // ── Active engine readiness signals for GUI ──────────────────
            json << "\"btc_move_bp\":" << balanced_.get_btc_move_bp() << ",";
            {
                double _sv = balanced_.get_session_vwap(i);
                double _vd = (_sv > 0 && ms.last_price > 0)
                    ? (_sv - ms.last_price) / _sv * 10000.0 : 0.0;
                json << "\"vwap_deviation_bp\":" << _vd << ",";
                json << "\"vwap_ready\":" << (_sv > 0 ? "true" : "false") << ",";
            }
            json << "\"mm_imbal_ema\":" << balanced_.get_mm_imbal_ema(i);
            
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
    OrderbookImbalanceEngine  obi_[MAX_SYMBOLS];
    AggressiveFlowEngine      afe_[MAX_SYMBOLS];
    PullbackContinuationEngine pce_[MAX_SYMBOLS];
    LiqBracketEngine           bracket_[MAX_SYMBOLS];
    BasisMomentumEngine        basis_[MAX_SYMBOLS];
    FundingWindowEngine        fund_win_[2];  // BTC=0, ETH=1 only
    std::vector<RegimeStateAllocator> allocator_;  // Use vector instead of array
    SimpleHttpServer http_server_;

    double last_latency_ms_  = 0.0;  // per-tick age for signal gating
    double lat_p95_display_  = 0.0;  // rolling p95 for GUI display only
    PerpFeed*            perp_feed_     = nullptr;  // optional -- set from main()

    // Global rate limiter for OBI/AFE/PCE: max 5 trades per 60s total
    int     micro_engine_trades_in_window_ = 0;
    int64_t micro_engine_window_start_ms_  = 0;
    static constexpr int   MICRO_ENGINE_MAX_PER_MIN  = 5;
    static constexpr int64_t MICRO_ENGINE_WINDOW_MS  = 60000;

    bool allow_micro_engine_trade(int64_t ts) {
        if (ts - micro_engine_window_start_ms_ > MICRO_ENGINE_WINDOW_MS) {
            micro_engine_trades_in_window_ = 0;
            micro_engine_window_start_ms_  = ts;
        }
        if (micro_engine_trades_in_window_ >= MICRO_ENGINE_MAX_PER_MIN) return false;
        micro_engine_trades_in_window_++;
        return true;
    }

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
    int    prev_obi_trades_[MAX_SYMBOLS]         = {};
    double prev_obi_pnl_[MAX_SYMBOLS]            = {};
    int    prev_afe_trades_[MAX_SYMBOLS]         = {};
    double prev_afe_pnl_[MAX_SYMBOLS]            = {};
    int    prev_pce_trades_[MAX_SYMBOLS]         = {};
    double prev_pce_pnl_[MAX_SYMBOLS]            = {};
    int    prev_bracket_trades_[MAX_SYMBOLS]     = {};
    double prev_bracket_pnl_[MAX_SYMBOLS]        = {};
    int    prev_basis_trades_[MAX_SYMBOLS]        = {};
    double prev_basis_pnl_[MAX_SYMBOLS]           = {};
    int    prev_fundwin_trades_[2]                 = {};
    double prev_fundwin_pnl_[2]                    = {};

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

        // OBI exit detected
        auto os = obi_[id].get_stats();
        if (os.total_trades > prev_obi_trades_[id]) {
            double trade_pnl = os.total_pnl_bp - prev_obi_pnl_[id];
            push_trade(sym_short(id), "OBI", trade_pnl, os.entry_price, px);
            prev_obi_trades_[id] = os.total_trades;
            prev_obi_pnl_[id]    = os.total_pnl_bp;
        }

        // AFE exit detected
        auto as = afe_[id].get_stats();
        if (as.total_trades > prev_afe_trades_[id]) {
            double trade_pnl = as.total_pnl_bp - prev_afe_pnl_[id];
            push_trade(sym_short(id), "AFE", trade_pnl, as.entry_price, px);
            prev_afe_trades_[id] = as.total_trades;
            prev_afe_pnl_[id]    = as.total_pnl_bp;
        }

        // PCE exit detected
        auto ps = pce_[id].get_stats();
        if (ps.total_trades > prev_pce_trades_[id]) {
            double trade_pnl = ps.total_pnl_bp - prev_pce_pnl_[id];
            push_trade(sym_short(id), "PCE", trade_pnl, ps.entry_price, px);
            prev_pce_trades_[id] = ps.total_trades;
            prev_pce_pnl_[id]    = ps.total_pnl_bp;
        }

        // FUNDING WINDOW exit detected (BTC/ETH only)
        if (id <= 1) {
            auto fw = fund_win_[id].get_stats();
            if (fw.total_trades > prev_fundwin_trades_[id]) {
                double trade_pnl = fw.total_pnl_bp - prev_fundwin_pnl_[id];
                push_trade(sym_short(id), "FUND-WIN", trade_pnl, fw.entry_price, px);
                prev_fundwin_trades_[id] = fw.total_trades;
                prev_fundwin_pnl_[id]    = fw.total_pnl_bp;
            }
        }

        // BASIS MOMENTUM exit detected
        auto bas = basis_[id].get_stats();
        if (bas.total_trades > prev_basis_trades_[id]) {
            double trade_pnl = bas.total_pnl_bp - prev_basis_pnl_[id];
            push_trade(sym_short(id), "BASIS", trade_pnl, bas.entry_price, px);
            prev_basis_trades_[id] = bas.total_trades;
            prev_basis_pnl_[id]    = bas.total_pnl_bp;
        }

        // BRACKET exit detected
        auto bks = bracket_[id].get_stats();
        if (bks.total_trades > prev_bracket_trades_[id]) {
            double trade_pnl = bks.total_pnl_bp - prev_bracket_pnl_[id];
            push_trade(sym_short(id), "BRACKET", trade_pnl, bks.entry_price, px);
            prev_bracket_trades_[id] = bks.total_trades;
            prev_bracket_pnl_[id]    = bks.total_pnl_bp;
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

        // Flow EMAs for AggressiveFlowEngine
        double buy_vol_ema  = 0.0;
        double sell_vol_ema = 0.0;
        bool   flow_init    = false;
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

        // Flow EMA update for AggressiveFlowEngine (alpha=0.05, ~20-tick window)
        // Note: called with tick.agg_buy_volume/sell_volume from on_tick
    }

    void update_flow_ema(int id, double buy_vol, double sell_vol) {
        auto& ms = market_state_[id];
        if (!ms.flow_init) {
            ms.buy_vol_ema  = buy_vol;
            ms.sell_vol_ema = sell_vol;
            ms.flow_init    = true;
        } else {
            ms.buy_vol_ema  = 0.95 * ms.buy_vol_ema  + 0.05 * buy_vol;
            ms.sell_vol_ema = 0.95 * ms.sell_vol_ema + 0.05 * sell_vol;
        }
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
