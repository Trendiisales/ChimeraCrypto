#pragma once
// ============================================================================
// EngineRegistry — Phase-4 review fix, item 20 (WIRING / REGISTRY).
//
// Replaces the hidden `CHIMERA_WIRE_LEGACY` env switch + the hardcoded /
// aspirational "N engines running" startup banner with an HONEST, machine-
// readable lifecycle registry that is RECONCILED against the actual engine
// graph at startup.
//
// Each engine (family / sleeve) DECLARES a lifecycle state:
//     DISABLED | SHADOW | PAPER | LIVE | HALTED | STALE
// loaded from config/engine_registry.json (hand-rolled parse, repo convention;
// programmatic declare() fallback when the file is absent). At wiring time the
// runtime calls mark_wired(name, connected, instances) to record what is
// ACTUALLY installed + callback-connected. validate() then FAILS (startup
// abort) if the declaration disagrees with reality — e.g. a "LIVE" entry whose
// callback is disconnected, or a "DISABLED" entry that is in fact wired live.
//
// The status/GUI engine count is generated from connected_count() — the real
// registered+connected graph — never a hardcoded number.
//
// Header-only, dependency-free (no curl/REST) so it compiles cheap + is unit-
// tested standalone. Everything crypto is SHADOW today.
// ============================================================================
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace chimera {

enum class Lifecycle { DISABLED, SHADOW, PAPER, LIVE, HALTED, STALE };

inline const char* lifecycle_str(Lifecycle s) {
    switch (s) {
        case Lifecycle::DISABLED: return "DISABLED";
        case Lifecycle::SHADOW:   return "SHADOW";
        case Lifecycle::PAPER:    return "PAPER";
        case Lifecycle::LIVE:     return "LIVE";
        case Lifecycle::HALTED:   return "HALTED";
        case Lifecycle::STALE:    return "STALE";
    }
    return "?";
}

inline bool lifecycle_parse(const std::string& in, Lifecycle& out) {
    std::string s; s.reserve(in.size());
    for (char c : in) s.push_back((char)std::toupper((unsigned char)c));
    if      (s == "DISABLED") out = Lifecycle::DISABLED;
    else if (s == "SHADOW")   out = Lifecycle::SHADOW;
    else if (s == "PAPER")    out = Lifecycle::PAPER;
    else if (s == "LIVE")     out = Lifecycle::LIVE;
    else if (s == "HALTED")   out = Lifecycle::HALTED;
    else if (s == "STALE")    out = Lifecycle::STALE;
    else return false;
    return true;
}

// An ACTIVE state asserts the engine is wired + producing/consuming callbacks.
// HALTED/STALE/DISABLED do NOT assert live trading.
inline bool lifecycle_is_active(Lifecycle s) {
    return s == Lifecycle::SHADOW || s == Lifecycle::PAPER || s == Lifecycle::LIVE;
}

struct RegEntry {
    std::string name;
    Lifecycle   declared  = Lifecycle::DISABLED;
    std::string note;
    bool        wired     = false;  // actually installed in the engine graph
    bool        connected = false;  // callback / data path actually attached
    int         instances = 0;      // concrete engine count (e.g. grid cells, slots)
};

class EngineRegistry {
public:
    // ---- declaration (programmatic fallback) --------------------------------
    void declare(const std::string& name, Lifecycle s, const std::string& note = "") {
        auto& e = entries_[name];
        e.name = name; e.declared = s;
        if (!note.empty()) e.note = note;
    }
    void set_state(const std::string& name, Lifecycle s) {
        auto it = entries_.find(name);
        if (it != entries_.end()) it->second.declared = s;
    }

    // ---- declared states from config/engine_registry.json -------------------
    // Format (array of objects; hand-rolled scan, order-independent):
    //   [ {"name":"XSEC-BTC","state":"SHADOW","note":"..."}, ... ]
    // Returns the number of entries loaded (0 => file absent/empty; caller then
    // relies on the programmatic declare() fallback).
    int load_from_json(const std::string& path) {
        std::ifstream f(path);
        if (!f) return 0;
        std::stringstream buf; buf << f.rdbuf();
        std::string s = buf.str();
        int loaded = 0;
        size_t pos = 0;
        while (true) {
            size_t nk = s.find("\"name\"", pos);
            if (nk == std::string::npos) break;
            std::string name = json_str_after_(s, nk);
            size_t obj_end = s.find('}', nk);
            if (obj_end == std::string::npos) obj_end = s.size();
            size_t sk = s.find("\"state\"", nk);
            Lifecycle st = Lifecycle::DISABLED;
            if (sk != std::string::npos && sk < obj_end) {
                std::string sv = json_str_after_(s, sk);
                lifecycle_parse(sv, st);
            }
            std::string note;
            size_t tk = s.find("\"note\"", nk);
            if (tk != std::string::npos && tk < obj_end) note = json_str_after_(s, tk);
            if (!name.empty()) { declare(name, st, note); ++loaded; }
            pos = obj_end + 1;
        }
        return loaded;
    }

    // ---- runtime wiring truth ----------------------------------------------
    void mark_wired(const std::string& name, bool connected, int instances = 1) {
        auto& e = entries_[name];
        e.name = name;
        e.wired = true;
        e.connected = connected;
        e.instances = instances;
    }

    // ---- reconcile declared vs actual --------------------------------------
    // Returns false + fills `err` on the FIRST mismatch. Rules:
    //  * ACTIVE (SHADOW/PAPER/LIVE) declared  => MUST be wired + connected,
    //    else overstatement ("declared LIVE but callback disconnected") -> abort.
    //  * DISABLED declared                    => MUST NOT be connected (the
    //    inverse overstatement: a supposedly-off engine is in fact live).
    //  * HALTED / STALE                       => operational; wiring allowed,
    //    trading not asserted -> never a mismatch.
    bool validate(std::string& err) const {
        for (const auto& kv : entries_) {
            const RegEntry& e = kv.second;
            if (lifecycle_is_active(e.declared)) {
                if (!e.wired || !e.connected) {
                    err = std::string("engine '") + e.name + "' declared "
                        + lifecycle_str(e.declared)
                        + " but " + (!e.wired ? "NOT WIRED" : "callback DISCONNECTED");
                    return false;
                }
            } else if (e.declared == Lifecycle::DISABLED) {
                if (e.connected) {
                    err = std::string("engine '") + e.name
                        + "' declared DISABLED but is actually CONNECTED (running)";
                    return false;
                }
            }
        }
        err.clear();
        return true;
    }

    // ---- honest counts (drive the status/GUI banner) -----------------------
    // Sum of concrete instances across ACTIVE + connected entries.
    int connected_count() const {
        int n = 0;
        for (const auto& kv : entries_)
            if (lifecycle_is_active(kv.second.declared) && kv.second.connected)
                n += std::max(1, kv.second.instances);
        return n;
    }
    int declared_active_count() const {
        int n = 0;
        for (const auto& kv : entries_)
            if (lifecycle_is_active(kv.second.declared)) ++n;
        return n;
    }
    int count_in_state(Lifecycle s) const {
        int n = 0;
        for (const auto& kv : entries_) if (kv.second.declared == s) ++n;
        return n;
    }

    std::vector<RegEntry> entries() const {
        std::vector<RegEntry> v;
        for (const auto& kv : entries_) v.push_back(kv.second);
        std::sort(v.begin(), v.end(),
                  [](const RegEntry& a, const RegEntry& b){ return a.name < b.name; });
        return v;
    }

    void print_summary(const char* prefix = "[REGISTRY]") const {
        for (const auto& e : entries()) {
            std::printf("%s %-18s declared=%-8s wired=%d connected=%d instances=%d%s%s\n",
                        prefix, e.name.c_str(), lifecycle_str(e.declared),
                        e.wired ? 1 : 0, e.connected ? 1 : 0, e.instances,
                        e.note.empty() ? "" : "  // ", e.note.c_str());
        }
        std::printf("%s connected_engines=%d (from the real graph, not a banner)\n",
                    prefix, connected_count());
        std::fflush(stdout);
    }

    // Compact JSON for /api/state2 (GUI reads the honest graph, not a constant).
    std::string state_json() const {
        std::ostringstream js;
        js << "{\"connected_count\":" << connected_count() << ",\"entries\":[";
        bool first = true;
        for (const auto& e : entries()) {
            if (!first) js << ",";
            first = false;
            js << "{\"name\":\"" << e.name << "\",\"state\":\"" << lifecycle_str(e.declared)
               << "\",\"wired\":" << (e.wired ? "true" : "false")
               << ",\"connected\":" << (e.connected ? "true" : "false")
               << ",\"instances\":" << e.instances << "}";
        }
        js << "]}";
        return js.str();
    }

private:
    // Return the string value of the "key": "value" starting near key position.
    static std::string json_str_after_(const std::string& s, size_t keypos) {
        size_t colon = s.find(':', keypos);
        if (colon == std::string::npos) return "";
        size_t q1 = s.find('"', colon);
        if (q1 == std::string::npos) return "";
        size_t q2 = s.find('"', q1 + 1);
        if (q2 == std::string::npos) return "";
        return s.substr(q1 + 1, q2 - q1 - 1);
    }

    std::map<std::string, RegEntry> entries_;
};

} // namespace chimera
