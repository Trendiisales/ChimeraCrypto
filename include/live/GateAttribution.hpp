#pragma once
// ============================================================================
// GateAttribution — Phase-4 review fix, item 21 (OBSERVABILITY).
//
// EdgeEngine's ~11 entry gates (portfolio / cluster / confirmation / funding /
// vol-regime / corr / session / volume / vol-filter / mtf / adx) each log a
// SUPPRESSED line and return — so we can see WHICH gate fired but NOT whether
// suppressing was USEFUL (did it dodge a loser, or kill a winner?).
//
// This sink records, for EVERY raw signal:
//   * a monotonic CORRELATION-ID (threaded signal -> target -> order -> fill
//     -> pnl so a decision is resolvable end-to-end),
//   * each gate's suppression reason,
//   * the COUNTERFACTUAL: what a long taken at the signal price WOULD have done
//     had the gate been ignored (resolved forward from live prices over a
//     horizon, with optional tp/sl). Aggregated per-gate this tells us which
//     gates suppress the SAME good trades vs which dodge losers.
//
// Purely observational: it records, it never changes signal/exit logic. Header-
// only + dependency-free (unit-tested standalone). Thread-safe (one mutex).
// ============================================================================
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <mutex>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace chimera {

struct GateHit {
    std::string gate;    // e.g. "PORTFOLIO_GATE"
    std::string reason;  // human-readable reason string
};

struct GateRecord {
    uint64_t     corr_id = 0;
    int64_t      ts_ms   = 0;
    std::string  tag;      // engine tag
    std::string  symbol;   // e.g. "btcusdt"
    std::string  kind;     // strategy kind string
    double       signal_px = 0.0;
    bool         suppressed = false;   // any gate blocked it
    bool         entered    = false;   // passed all gates (real entry taken)
    std::string  first_gate;           // gate of the FIRST suppression
    std::vector<GateHit> hits;         // every gate reason recorded on this signal

    // Counterfactual (resolved forward from prices; only meaningful if suppressed).
    bool    cf_open     = false;       // awaiting resolution
    bool    cf_resolved = false;
    double  cf_exit_px  = 0.0;
    double  cf_return_bp = 0.0;        // hypothetical long return, gate ignored
    int64_t cf_exit_ts  = 0;
    const char* cf_reason = "";        // "HORIZON" | "TP" | "SL"
};

struct GateStat {
    int    suppressed   = 0;   // signals this gate blocked
    int    cf_resolved  = 0;   // counterfactuals resolved
    int    cf_positive  = 0;   // resolved counterfactuals that were GOOD trades (>0)
    double cf_sum_bp    = 0.0; // sum of counterfactual returns (bp)
    double avg_bp() const { return cf_resolved > 0 ? cf_sum_bp / cf_resolved : 0.0; }
    // "helpfulness": a gate is HELPFUL when the trades it suppressed lost on
    // average (avg_bp < 0); HARMFUL when it suppressed winners (avg_bp > 0).
    bool   helpful() const { return cf_resolved > 0 && avg_bp() < 0.0; }
};

class GateAttribution {
public:
    // horizon_ms: how long to hold the hypothetical before a HORIZON resolve.
    // tp_bp/sl_bp (>0): resolve early on a take-profit / stop-loss touch.
    void configure(int64_t horizon_ms, double tp_bp = 0.0, double sl_bp = 0.0) {
        horizon_ms_ = horizon_ms; tp_bp_ = tp_bp; sl_bp_ = sl_bp;
    }

    // Bound the in-memory per-signal store to at most `max_records` records with
    // OLDEST-FIRST eviction (a FIFO ring). 0 = unbounded (legacy default). The
    // aggregated per-gate counterfactual stats (per_gate_) are PRESERVED across
    // eviction — only the granular per-signal detail is bounded. This caps the
    // ~15MB/month unbounded growth over a long shadow run while keeping the
    // rolling counterfactual research value intact. Thread-safe.
    void set_capacity(size_t max_records) {
        std::lock_guard<std::mutex> lk(mtx_);
        cap_ = max_records;
        evict_oldest_locked();
    }
    size_t capacity() const { std::lock_guard<std::mutex> lk(mtx_); return cap_; }
    size_t evicted() const  { std::lock_guard<std::mutex> lk(mtx_); return evicted_; }

    // Begin a raw signal -> returns its correlation id. Call once per raw fire.
    uint64_t begin_signal(const std::string& tag, const std::string& symbol,
                          const std::string& kind, double signal_px, int64_t ts_ms) {
        std::lock_guard<std::mutex> lk(mtx_);
        uint64_t id = ++seq_;
        GateRecord r;
        r.corr_id = id; r.ts_ms = ts_ms; r.tag = tag; r.symbol = symbol;
        r.kind = kind; r.signal_px = signal_px;
        records_[id] = r;
        order_.push_back(id);
        evict_oldest_locked();
        return id;
    }

    // A gate suppressed this signal. Records the reason + opens the
    // counterfactual (what it would have done, ignored).
    void suppressed(uint64_t corr_id, const std::string& gate, const std::string& reason) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = records_.find(corr_id);
        if (it == records_.end()) return;
        GateRecord& r = it->second;
        r.hits.push_back({gate, reason});
        if (!r.suppressed) { r.suppressed = true; r.first_gate = gate; }
        // Open a counterfactual keyed on the FIRST suppressing gate (the one
        // that actually blocked the trade in the real chain).
        if (!r.cf_open && !r.cf_resolved && r.signal_px > 0.0) {
            r.cf_open = true;
            open_cf_.push_back(corr_id);
        }
        auto& st = per_gate_[gate];
        st.suppressed++;
    }

    // The signal passed every gate and a real entry was taken.
    void passed(uint64_t corr_id) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = records_.find(corr_id);
        if (it != records_.end()) it->second.entered = true;
    }

    // Feed forward prices; resolves any open counterfactual for `symbol`.
    void on_price(const std::string& symbol, double px, int64_t ts_ms) {
        if (px <= 0.0) return;
        std::lock_guard<std::mutex> lk(mtx_);
        std::vector<uint64_t> still;
        still.reserve(open_cf_.size());
        for (uint64_t id : open_cf_) {
            auto it = records_.find(id);
            if (it == records_.end()) continue;
            GateRecord& r = it->second;
            if (!r.cf_open || r.symbol != symbol) { if (r.cf_open) still.push_back(id); continue; }
            double ret_bp = (px / r.signal_px - 1.0) * 1e4;
            const char* why = nullptr;
            if (tp_bp_ > 0.0 && ret_bp >= tp_bp_)                          why = "TP";
            else if (sl_bp_ > 0.0 && ret_bp <= -sl_bp_)                    why = "SL";
            else if (horizon_ms_ > 0 && ts_ms - r.ts_ms >= horizon_ms_)   why = "HORIZON";
            if (why) {
                r.cf_open = false; r.cf_resolved = true;
                r.cf_exit_px = px; r.cf_return_bp = ret_bp; r.cf_exit_ts = ts_ms;
                r.cf_reason = why;
                // credit the first suppressing gate's stats
                if (!r.first_gate.empty()) {
                    auto& st = per_gate_[r.first_gate];
                    st.cf_resolved++; st.cf_sum_bp += ret_bp;
                    if (ret_bp > 0.0) st.cf_positive++;
                }
            } else {
                still.push_back(id);
            }
        }
        open_cf_.swap(still);
    }

    // Resolve any still-open counterfactual at a final price (end-of-run flush).
    void flush(const std::map<std::string,double>& last_px, int64_t ts_ms) {
        std::lock_guard<std::mutex> lk(mtx_);
        for (uint64_t id : open_cf_) {
            auto it = records_.find(id);
            if (it == records_.end()) continue;
            GateRecord& r = it->second;
            auto pit = last_px.find(r.symbol);
            if (pit == last_px.end() || pit->second <= 0.0) continue;
            double ret_bp = (pit->second / r.signal_px - 1.0) * 1e4;
            r.cf_open = false; r.cf_resolved = true;
            r.cf_exit_px = pit->second; r.cf_return_bp = ret_bp;
            r.cf_exit_ts = ts_ms; r.cf_reason = "FLUSH";
            if (!r.first_gate.empty()) {
                auto& st = per_gate_[r.first_gate];
                st.cf_resolved++; st.cf_sum_bp += ret_bp;
                if (ret_bp > 0.0) st.cf_positive++;
            }
        }
        open_cf_.clear();
    }

    // Resolve a correlation id -> the full decision record (nullptr if unknown).
    const GateRecord* find(uint64_t corr_id) const {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = records_.find(corr_id);
        return it == records_.end() ? nullptr : &it->second;
    }

    std::map<std::string, GateStat> per_gate_stats() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return per_gate_;
    }

    size_t size() const { std::lock_guard<std::mutex> lk(mtx_); return records_.size(); }
    size_t open_counterfactuals() const { std::lock_guard<std::mutex> lk(mtx_); return open_cf_.size(); }

    void print_summary(const char* prefix = "[GATE-ATTR]") const {
        std::lock_guard<std::mutex> lk(mtx_);
        std::printf("%s %zu signals in-store (cap=%zu, %zu evicted oldest-first), "
                    "per-gate helpfulness (avg counterfactual bp):\n",
                    prefix, records_.size(), cap_, evicted_);
        for (const auto& kv : per_gate_) {
            const GateStat& st = kv.second;
            std::printf("%s   %-18s suppressed=%d cf_resolved=%d cf_pos=%d avg=%+.1fbp -> %s\n",
                        prefix, kv.first.c_str(), st.suppressed, st.cf_resolved,
                        st.cf_positive, st.avg_bp(),
                        st.cf_resolved == 0 ? "n/a"
                          : (st.helpful() ? "HELPFUL (dodged losers)"
                                          : "SUSPECT (killed winners)"));
        }
        std::fflush(stdout);
    }

private:
    // Evict oldest records (FIFO by insertion order) until size <= cap_.
    // Caller MUST hold mtx_. Aggregated per_gate_ stats are intentionally left
    // untouched — they are the retained research value; only granular per-signal
    // records are bounded. Any evicted record with a still-open counterfactual
    // is dropped from open_cf_ so that vector stays bounded too.
    void evict_oldest_locked() {
        while (cap_ > 0 && records_.size() > cap_ && !order_.empty()) {
            uint64_t old = order_.front();
            order_.pop_front();
            auto it = records_.find(old);
            if (it == records_.end()) continue;   // defensive: already gone
            if (it->second.cf_open) {
                auto p = std::find(open_cf_.begin(), open_cf_.end(), old);
                if (p != open_cf_.end()) open_cf_.erase(p);
            }
            records_.erase(it);
            ++evicted_;
        }
    }

    mutable std::mutex mtx_;
    uint64_t seq_ = 0;
    int64_t  horizon_ms_ = 0;
    double   tp_bp_ = 0.0, sl_bp_ = 0.0;
    size_t   cap_ = 0;        // 0 = unbounded (legacy). set_capacity() to bound.
    size_t   evicted_ = 0;    // count of oldest-first evictions (observability).
    std::map<uint64_t, GateRecord> records_;
    std::deque<uint64_t>           order_;    // FIFO insertion order for eviction.
    std::vector<uint64_t>          open_cf_;
    std::map<std::string, GateStat> per_gate_;
};

} // namespace chimera
