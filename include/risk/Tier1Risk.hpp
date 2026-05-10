#pragma once
// ============================================================================
// Tier1Risk.hpp
// Chimera — Tier 1 risk wrapper (single-header, header-only)
//
// Required spec from HANDOFF_MOVE2.md:
//   "daily loss circuit, correlation-aware sizing, per-engine kill,
//    state persistence, reconciliation"
//
// PURPOSE
//   Replace the hardcoded `available_R = 1.0` placeholder threaded through
//   every engine's on_tick() in src/main.cpp with a centrally-managed
//   risk budget that:
//
//     1. Caps per-engine open exposure (per_engine_r_cap).
//     2. Caps total open exposure across all engines (total_r_cap).
//     3. Enforces a daily realized-loss circuit (daily_loss_kill_bp).
//     4. Imposes a correlation cap so N engines can't all take the same
//        directional bet on the same symbol at full size.
//     5. Imposes a per-engine order rate limit.
//     6. Persists daily P&L + halt state across process restarts so a
//        crash doesn't reset the daily circuit.
//
// ENGINE INTEGRATION (when ready to flip an engine live, NOT in this file)
//   In src/main.cpp, the existing call:
//       engine.on_tick(id, tick, now_ms, /*available_R=*/1.0);
//   becomes:
//       const double R = risk.available_R(EngineType::ETH_BTC_LEADLAG, id);
//       engine.on_tick(id, tick, now_ms, R);
//   And the engine's executor wiring also calls:
//       risk.on_position_open (eng, id, is_long, size_R);  // on entry
//       risk.on_position_close(eng, id, realized_pnl_bp); // on exit
//
//   Engines that read available_R == 0.0 already no-op cleanly (every
//   _try_enter() in the existing engines starts with `if (available_R <
//   MIN_AVAIL_R) return;`).
//
// THREAD SAFETY
//   All public methods take `mu_`. Read-mostly accessors (available_R,
//   is_halted, snapshot) take it briefly; mutators hold it across the
//   full state update + persistence write. Persistence write is small
//   (~500 bytes) and infrequent (only on entry/exit/halt) so the mutex
//   hold time is dominated by file I/O. If that ever shows up as a hot
//   path the persistence write can be moved to a worker thread.
//
// PERSISTENCE
//   JSON written to cfg_.state_path (default: data/tier1_risk_state.json).
//   Written on every state-changing call. Read once in load_state().
//   If the file is corrupt or missing, load_state() leaves defaults
//   (no daily loss recorded, not halted, no positions). Conservative:
//   a corrupt state file MUST NOT bypass the daily kill — but a fresh
//   file can't have a daily loss to honour, so starting from zero is
//   the right behaviour.
//
// TIME INJECTION
//   set_clock(fn) lets tests inject a deterministic millisecond clock.
//   Default is wall-clock UTC (via std::chrono::system_clock). All
//   timestamps in the persistence file are wall-clock UTC ms since
//   epoch — so a test that injects a fake clock followed by a real
//   process restart will observe a stale state file but recover safely.
// ============================================================================

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>

namespace chimera::risk {

// ── Engine identity ─────────────────────────────────────────────────────────
enum class EngineType : int {
    SWING                     = 0,
    FUNDING_WINDOW            = 1,
    BASIS_MOMENTUM            = 2,
    OBI                       = 3,
    ETH_BTC_LEADLAG           = 4,
    COINBASE_PREMIUM_MREV     = 5,
    FUNDING_PERSIST_FADE      = 6,
    VOL_COMPRESSION_BREAKOUT  = 7,
    NUM_ENGINES               = 8
};

inline const char* engine_name(EngineType e) {
    switch (e) {
        case EngineType::SWING:                    return "SWING";
        case EngineType::FUNDING_WINDOW:           return "FUNDING_WINDOW";
        case EngineType::BASIS_MOMENTUM:           return "BASIS_MOMENTUM";
        case EngineType::OBI:                      return "OBI";
        case EngineType::ETH_BTC_LEADLAG:          return "ETH_BTC_LEADLAG";
        case EngineType::COINBASE_PREMIUM_MREV:    return "CB_PREMIUM_MREV";
        case EngineType::FUNDING_PERSIST_FADE:     return "FUNDING_PERSIST";
        case EngineType::VOL_COMPRESSION_BREAKOUT: return "VOL_COMPRESS";
        default:                                   return "UNKNOWN";
    }
}

class Tier1Risk {
public:
    static constexpr int N_ENGINES = (int)EngineType::NUM_ENGINES;
    static constexpr int MAX_SYMS  = 8;   // matches MAX_SYMBOLS in SymbolIndex

    struct Config {
        // Per-engine maximum simultaneous size_R. Default 1.0 = matches
        // the current placeholder. Tighten per-engine before flipping live.
        std::array<double, N_ENGINES> per_engine_r_cap;

        // Total open exposure cap (sum of size_R across all engines).
        // Default 3.0 = three full-size positions tolerated concurrently.
        double total_r_cap = 3.0;

        // Daily realized-loss kill threshold, in bp of total session P&L.
        // Negative number. When daily_realized_bp <= daily_loss_kill_bp,
        // halt_all is invoked with reason "daily_loss_circuit_tripped".
        double daily_loss_kill_bp = -200.0;

        // Correlation cap: max number of engines allowed to hold ANY
        // exposure on the same (symbol, side). Set to 1 for strictest
        // correlation control; 8 (= N_ENGINES) effectively disables.
        int max_engines_per_symbol_side = 2;

        // Per-engine order rate limit. on_position_open() will be denied
        // if the engine has already opened max_orders_per_minute positions
        // in the current rolling minute.
        int max_orders_per_minute = 10;

        // Persistence file path. "" disables persistence.
        std::string state_path = "data/tier1_risk_state.json";

        Config() {
            per_engine_r_cap.fill(1.0);
        }
    };

    // ── Decision context returned by authorize_entry() ──────────────────────
    struct Decision {
        bool        ok         = false;
        double      granted_R  = 0.0;     // size_R the engine is permitted (≤ requested)
        const char* reason     = "ok";
    };

    explicit Tier1Risk(const Config& cfg = Config{}) : cfg_(cfg) {
        // Default clock = wall clock UTC ms.
        clock_ = []() -> int64_t {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        };
        for (int i = 0; i < N_ENGINES; ++i) {
            positions_[i] = PositionState{};
            orders_this_minute_[i] = 0;
        }
        _load_state_unlocked();
    }

    // ── Available R for an engine on a given symbol ────────────────────────
    // Engines should call this on every tick and pass the result as the
    // available_R argument to on_tick(). Returns 0.0 when entry would be
    // denied for ANY reason (halted, daily kill, per-engine cap reached,
    // correlation cap reached, total exposure cap reached). Returns a
    // positive number ≤ per_engine_r_cap when entry is permitted.
    double available_R(EngineType engine, int symbol_id) const {
        std::lock_guard<std::mutex> g(mu_);
        return _available_R_unlocked(engine, symbol_id, /*is_long_hint=*/true);
    }

    // ── Authorize a specific entry intent ──────────────────────────────────
    // Engines that want a yes/no with reason should call this just before
    // submitting an order. Returns granted_R (may be < requested_R if a
    // cap clips it). Does NOT update state — call on_position_open()
    // after the order actually fills.
    Decision authorize_entry(EngineType engine, int symbol_id, bool is_long,
                             double requested_R) const {
        std::lock_guard<std::mutex> g(mu_);
        Decision d;

        if (halted_) { d.reason = "halted"; return d; }

        const int ei = (int)engine;
        if (ei < 0 || ei >= N_ENGINES) { d.reason = "bad_engine"; return d; }
        if (symbol_id < 0 || symbol_id >= MAX_SYMS) {
            d.reason = "bad_symbol"; return d;
        }
        if (requested_R <= 0.0) { d.reason = "non_positive_R"; return d; }

        // Per-engine R cap (already-open exposure + requested ≤ cap)
        const double already = positions_[ei].size_R;
        const double pe_cap  = cfg_.per_engine_r_cap[ei];
        const double pe_room = pe_cap - already;
        if (pe_room <= 0.0) { d.reason = "per_engine_cap"; return d; }

        // Total exposure cap
        double total = 0.0;
        for (int i = 0; i < N_ENGINES; ++i) total += positions_[i].size_R;
        const double tot_room = cfg_.total_r_cap - total;
        if (tot_room <= 0.0) { d.reason = "total_R_cap"; return d; }

        // Correlation cap
        int corr = 0;
        for (int i = 0; i < N_ENGINES; ++i) {
            if (positions_[i].size_R <= 0.0) continue;
            if (positions_[i].symbol_id == symbol_id &&
                positions_[i].is_long   == is_long) {
                ++corr;
            }
        }
        if (corr >= cfg_.max_engines_per_symbol_side) {
            d.reason = "correlation_cap"; return d;
        }

        // Order rate limit
        if (orders_this_minute_[ei] >= cfg_.max_orders_per_minute) {
            d.reason = "rate_limit"; return d;
        }

        d.ok = true;
        d.granted_R = std::min({requested_R, pe_room, tot_room});
        return d;
    }

    // ── State updates: call on actual fill / actual close ──────────────────
    void on_position_open(EngineType engine, int symbol_id, bool is_long,
                          double size_R) {
        std::lock_guard<std::mutex> g(mu_);
        const int ei = (int)engine;
        if (ei < 0 || ei >= N_ENGINES) return;
        if (symbol_id < 0 || symbol_id >= MAX_SYMS) return;
        if (size_R <= 0.0) return;

        _maybe_roll_minute_unlocked();

        positions_[ei].is_long   = is_long;
        positions_[ei].size_R   += size_R;
        positions_[ei].symbol_id = symbol_id;
        orders_this_minute_[ei] += 1;

        _save_state_unlocked();
    }

    void on_position_close(EngineType engine, double realized_pnl_bp) {
        std::lock_guard<std::mutex> g(mu_);
        const int ei = (int)engine;
        if (ei < 0 || ei >= N_ENGINES) return;

        _maybe_roll_day_unlocked();

        positions_[ei] = PositionState{};   // zero out
        daily_realized_bp_ += realized_pnl_bp;

        // Daily loss circuit
        if (daily_realized_bp_ <= cfg_.daily_loss_kill_bp && !halted_) {
            halted_ = true;
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                "daily_loss_circuit_tripped: realized=%.2fbp threshold=%.2fbp",
                daily_realized_bp_, cfg_.daily_loss_kill_bp);
            halt_reason_ = buf;
            std::printf("[TIER1-RISK] HALTED — %s\n", halt_reason_.c_str());
            std::fflush(stdout);
        }

        _save_state_unlocked();
    }

    // ── Manual halt / resume ───────────────────────────────────────────────
    void halt_all(const std::string& reason) {
        std::lock_guard<std::mutex> g(mu_);
        if (halted_) return;
        halted_      = true;
        halt_reason_ = reason;
        std::printf("[TIER1-RISK] HALTED (manual) — %s\n", reason.c_str());
        std::fflush(stdout);
        _save_state_unlocked();
    }

    void resume_all() {
        std::lock_guard<std::mutex> g(mu_);
        if (!halted_) return;
        halted_      = false;
        halt_reason_ = "";
        std::printf("[TIER1-RISK] RESUMED\n");
        std::fflush(stdout);
        _save_state_unlocked();
    }

    bool is_halted() const {
        std::lock_guard<std::mutex> g(mu_);
        return halted_;
    }

    std::string halt_reason() const {
        std::lock_guard<std::mutex> g(mu_);
        return halt_reason_;
    }

    // ── Snapshot for /api/state2 / GUI / logging ───────────────────────────
    struct Snapshot {
        bool        halted;
        std::string halt_reason;
        double      daily_realized_bp;
        double      total_open_R;
        std::array<double, N_ENGINES> per_engine_open_R;
        std::array<int,    N_ENGINES> orders_this_minute;
    };

    Snapshot snapshot() const {
        std::lock_guard<std::mutex> g(mu_);
        Snapshot s{};
        s.halted             = halted_;
        s.halt_reason        = halt_reason_;
        s.daily_realized_bp  = daily_realized_bp_;
        s.total_open_R       = 0.0;
        for (int i = 0; i < N_ENGINES; ++i) {
            s.per_engine_open_R[i]   = positions_[i].size_R;
            s.orders_this_minute[i]  = orders_this_minute_[i];
            s.total_open_R          += positions_[i].size_R;
        }
        return s;
    }

    std::string snapshot_json() const {
        const Snapshot s = snapshot();
        std::ostringstream o;
        o << "{\"halted\":" << (s.halted ? "true" : "false")
          << ",\"halt_reason\":\"" << _escape_json(s.halt_reason) << "\""
          << ",\"daily_realized_bp\":" << s.daily_realized_bp
          << ",\"total_open_R\":"      << s.total_open_R
          << ",\"daily_loss_kill_bp\":" << cfg_.daily_loss_kill_bp
          << ",\"per_engine_open_R\":{";
        for (int i = 0; i < N_ENGINES; ++i) {
            if (i) o << ",";
            o << "\"" << engine_name((EngineType)i) << "\":"
              << s.per_engine_open_R[i];
        }
        o << "}}";
        return o.str();
    }

    // ── Time injection (tests) ─────────────────────────────────────────────
    void set_clock(std::function<int64_t()> clk) {
        std::lock_guard<std::mutex> g(mu_);
        clock_ = std::move(clk);
    }

    // ── Diagnostic for tests (force the day-rollover check) ────────────────
    void poke_clock_for_test() {
        std::lock_guard<std::mutex> g(mu_);
        _maybe_roll_day_unlocked();
        _maybe_roll_minute_unlocked();
    }

private:
    // ── Internal state ─────────────────────────────────────────────────────
    struct PositionState {
        bool   is_long   = false;
        double size_R    = 0.0;
        int    symbol_id = -1;
    };

    Config                    cfg_;
    mutable std::mutex        mu_;
    std::function<int64_t()>  clock_;

    bool                      halted_              = false;
    std::string               halt_reason_;
    int64_t                   day_started_ms_      = 0;
    double                    daily_realized_bp_   = 0.0;
    int64_t                   minute_started_ms_   = 0;

    std::array<PositionState, N_ENGINES> positions_{};
    std::array<int,           N_ENGINES> orders_this_minute_{};

    // ── Internal helpers (must be called with mu_ held) ────────────────────
    double _available_R_unlocked(EngineType engine, int symbol_id,
                                 bool is_long_hint) const {
        if (halted_) return 0.0;
        const int ei = (int)engine;
        if (ei < 0 || ei >= N_ENGINES) return 0.0;
        if (symbol_id < 0 || symbol_id >= MAX_SYMS) return 0.0;

        const double pe_cap = cfg_.per_engine_r_cap[ei];
        const double pe_room = pe_cap - positions_[ei].size_R;
        if (pe_room <= 0.0) return 0.0;

        double total = 0.0;
        for (int i = 0; i < N_ENGINES; ++i) total += positions_[i].size_R;
        const double tot_room = cfg_.total_r_cap - total;
        if (tot_room <= 0.0) return 0.0;

        int corr = 0;
        for (int i = 0; i < N_ENGINES; ++i) {
            if (positions_[i].size_R <= 0.0) continue;
            if (positions_[i].symbol_id == symbol_id &&
                positions_[i].is_long   == is_long_hint) {
                ++corr;
            }
        }
        if (corr >= cfg_.max_engines_per_symbol_side) return 0.0;

        if (orders_this_minute_[ei] >= cfg_.max_orders_per_minute) return 0.0;

        return std::min(pe_room, tot_room);
    }

    static int64_t _utc_day_start_ms(int64_t now_ms) {
        const int64_t day_ms = 86'400'000LL;
        return (now_ms / day_ms) * day_ms;
    }

    void _maybe_roll_day_unlocked() {
        const int64_t now = clock_();
        const int64_t today_start = _utc_day_start_ms(now);
        if (today_start > day_started_ms_) {
            day_started_ms_    = today_start;
            daily_realized_bp_ = 0.0;
            // Note: we do NOT auto-resume on day rollover. If the daily
            // circuit tripped yesterday, an operator must call resume_all()
            // manually after reviewing what happened. This is intentional.
        }
    }

    void _maybe_roll_minute_unlocked() {
        const int64_t now = clock_();
        const int64_t minute_start = (now / 60'000LL) * 60'000LL;
        if (minute_start > minute_started_ms_) {
            minute_started_ms_ = minute_start;
            for (int i = 0; i < N_ENGINES; ++i) orders_this_minute_[i] = 0;
        }
    }

    static std::string _escape_json(const std::string& s) {
        std::string out;
        out.reserve(s.size() + 4);
        for (char c : s) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if ((unsigned char)c < 0x20) {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    } else {
                        out += c;
                    }
            }
        }
        return out;
    }

    void _save_state_unlocked() {
        if (cfg_.state_path.empty()) return;
        std::ostringstream o;
        o << "{\n"
          << "  \"day_started_ms\":"     << day_started_ms_      << ",\n"
          << "  \"daily_realized_bp\":"  << daily_realized_bp_   << ",\n"
          << "  \"halted\":"             << (halted_ ? "true" : "false") << ",\n"
          << "  \"halt_reason\":\""      << _escape_json(halt_reason_) << "\",\n"
          << "  \"positions\":[";
        for (int i = 0; i < N_ENGINES; ++i) {
            if (i) o << ",";
            o << "{\"engine\":\"" << engine_name((EngineType)i) << "\""
              << ",\"size_R\":"   << positions_[i].size_R
              << ",\"is_long\":"  << (positions_[i].is_long ? "true" : "false")
              << ",\"symbol_id\":" << positions_[i].symbol_id << "}";
        }
        o << "]\n}\n";

        std::ofstream f(cfg_.state_path);
        if (!f) return;            // silent failure — don't block the trade
        f << o.str();
    }

    // ── Minimal hand-rolled JSON parser for the small known schema we write ─
    // We control the writer, so we don't need a general-purpose parser. The
    // parser is forgiving — any field it can't find leaves the default in
    // place. This means a corrupt or partially-written state file degrades
    // to "fresh start" rather than crashing.
    void _load_state_unlocked() {
        if (cfg_.state_path.empty()) return;
        std::ifstream f(cfg_.state_path);
        if (!f) return;
        std::ostringstream ss;
        ss << f.rdbuf();
        const std::string src = ss.str();
        if (src.empty()) return;

        day_started_ms_    = (int64_t)_extract_dbl(src, "day_started_ms", 0.0);
        daily_realized_bp_ = _extract_dbl(src, "daily_realized_bp", 0.0);
        halted_            = _extract_bool(src, "halted", false);
        halt_reason_       = _extract_str(src, "halt_reason", "");

        // Positions: parse each "engine" inside the positions array.
        // The order is fixed (matches NUM_ENGINES) so we look for each
        // engine name and pull its size_R / is_long / symbol_id.
        for (int i = 0; i < N_ENGINES; ++i) {
            const std::string tag = std::string("\"engine\":\"") +
                                    engine_name((EngineType)i) + "\"";
            const auto pos = src.find(tag);
            if (pos == std::string::npos) continue;
            // Parse the {...} containing this tag.
            const auto obj_start = src.rfind('{', pos);
            const auto obj_end   = src.find('}', pos);
            if (obj_start == std::string::npos || obj_end == std::string::npos)
                continue;
            const std::string obj = src.substr(obj_start, obj_end - obj_start);
            positions_[i].size_R    = _extract_dbl (obj, "size_R",   0.0);
            positions_[i].is_long   = _extract_bool(obj, "is_long",  false);
            positions_[i].symbol_id = (int)_extract_dbl(obj, "symbol_id", -1.0);
        }

        std::printf("[TIER1-RISK] State loaded from %s "
                    "(daily=%.2fbp halted=%d)\n",
                    cfg_.state_path.c_str(), daily_realized_bp_, halted_ ? 1 : 0);
        std::fflush(stdout);
    }

    static double _extract_dbl(const std::string& s, const std::string& key,
                               double dflt) {
        const std::string nb = "\"" + key + "\":";
        auto p = s.find(nb);
        if (p == std::string::npos) return dflt;
        p += nb.size();
        // skip optional whitespace
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t' ||
                                s[p] == '\n' || s[p] == '\r')) ++p;
        // numeric only — caller asks for a number, ignore quoted values
        auto e = s.find_first_of(",}\n\r ", p);
        if (e == std::string::npos) e = s.size();
        try { return std::stod(s.substr(p, e - p)); }
        catch (...) { return dflt; }
    }

    static bool _extract_bool(const std::string& s, const std::string& key,
                              bool dflt) {
        const std::string nb = "\"" + key + "\":";
        auto p = s.find(nb);
        if (p == std::string::npos) return dflt;
        p += nb.size();
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t' ||
                                s[p] == '\n' || s[p] == '\r')) ++p;
        if (s.compare(p, 4, "true")  == 0) return true;
        if (s.compare(p, 5, "false") == 0) return false;
        return dflt;
    }

    static std::string _extract_str(const std::string& s, const std::string& key,
                                    const std::string& dflt) {
        const std::string nq = "\"" + key + "\":\"";
        auto p = s.find(nq);
        if (p == std::string::npos) return dflt;
        p += nq.size();
        // simple read until next unescaped quote
        std::string out;
        while (p < s.size() && s[p] != '"') {
            if (s[p] == '\\' && p + 1 < s.size()) {
                char c = s[p+1];
                switch (c) {
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    default: out += c; break;
                }
                p += 2;
            } else {
                out += s[p++];
            }
        }
        return out;
    }
};

} // namespace chimera::risk
