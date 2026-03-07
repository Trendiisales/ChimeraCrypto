#pragma once
// ============================================================================
// ShadowLogger — structured trade journal for edge measurement
// ============================================================================
// Records every entry and exit with full context. After 300+ trades you can
// measure:
//   - Win rate per strategy
//   - Actual MFE (did price reach TP?) vs theoretical TP
//   - Average hold time per strategy
//   - P&L per strategy after real costs
//   - Whether order-flow confirmation improves win rate
//
// CSV format: one row per trade exit
// Fields: ts_enter,ts_exit,symbol,layer,regime,entry_px,exit_px,
//         pnl_bp,mfe_bp,mae_bp,hold_ms,latency_ms,
//         imbalance_at_entry,flow_ratio_at_entry,spread_at_entry,
//         btc_move_bp,win
// ============================================================================
#include <cstdio>
#include <cstdint>
#include <cstring>

namespace chimera {

struct ShadowEntry {
    int64_t ts_enter       = 0;
    int64_t ts_exit        = 0;
    char    symbol[8]      = {};
    char    layer[12]      = {};
    char    regime[12]     = {};
    double  entry_px       = 0.0;
    double  exit_px        = 0.0;
    double  pnl_bp         = 0.0;
    double  mfe_bp         = 0.0;
    double  mae_bp         = 0.0;
    int64_t hold_ms        = 0;
    double  latency_ms     = 0.0;
    double  imbalance      = 0.0;   // book_imbalance at entry
    double  flow_ratio     = 0.0;   // agg_buy/(agg_buy+agg_sell) at entry, 0.5=neutral
    double  spread_bps     = 0.0;   // spread at entry
    double  btc_move_bp    = 0.0;   // BTC move in last 100ms at entry (lead-lag context)
    int     win            = 0;     // 1=win 0=loss
};

class ShadowLogger {
public:
    ShadowLogger() {
        f_ = std::fopen("/tmp/chimera_shadow.csv", "a");
        if (f_) {
            // Write header if file is empty
            std::fseek(f_, 0, SEEK_END);
            if (std::ftell(f_) == 0) {
                std::fprintf(f_,
                    "ts_enter,ts_exit,symbol,layer,regime,"
                    "entry_px,exit_px,pnl_bp,mfe_bp,mae_bp,"
                    "hold_ms,latency_ms,imbalance,flow_ratio,"
                    "spread_bps,btc_move_bp,win\n");
            }
            std::fflush(f_);
        }
    }

    ~ShadowLogger() {
        if (f_) std::fclose(f_);
    }

    void record(const ShadowEntry& e) {
        if (!f_) return;
        std::fprintf(f_,
            "%lld,%lld,%s,%s,%s,"
            "%.4f,%.4f,%.4f,%.4f,%.4f,"
            "%lld,%.2f,%.4f,%.4f,"
            "%.4f,%.4f,%d\n",
            (long long)e.ts_enter, (long long)e.ts_exit,
            e.symbol, e.layer, e.regime,
            e.entry_px, e.exit_px, e.pnl_bp, e.mfe_bp, e.mae_bp,
            (long long)e.hold_ms, e.latency_ms, e.imbalance, e.flow_ratio,
            e.spread_bps, e.btc_move_bp, e.win);
        std::fflush(f_);
    }

private:
    FILE* f_ = nullptr;
};

} // namespace chimera
