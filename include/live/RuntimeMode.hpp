#pragma once
// ============================================================================
// RuntimeMode — ONE immutable process-wide execution mode.
//
// Phase-1 review fix (2026-07-11): the mode was previously spread across
// live_config.json (shadow_mode bool) + binance_credentials.json (shadow_mode)
// + a testing-bypass flag, with NO cross-file consistency check. A disagreement
// (e.g. live_config shadow=true but credentials shadow=false) could route real
// orders while the rest of the system believed it was in shadow.
//
// This defines a single enum resolved once at startup and cross-checked against
// every mode-bearing config file; a disagreement is a HARD startup abort.
//   DISABLED  — never trades (not even shadow logging of orders)
//   SHADOW    — signs + logs, never POSTs (default)
//   PAPER     — like shadow, reserved for a future paper-fill simulator
//   LIVE      — signs + POSTs real exchange orders
// Only LIVE may reach a real POST.
// ============================================================================
#include <string>
#include <cctype>

namespace chimera {

enum class RuntimeMode { DISABLED, SHADOW, PAPER, LIVE };

inline const char* runtime_mode_str(RuntimeMode m) {
    switch (m) {
        case RuntimeMode::DISABLED: return "DISABLED";
        case RuntimeMode::SHADOW:   return "SHADOW";
        case RuntimeMode::PAPER:    return "PAPER";
        case RuntimeMode::LIVE:     return "LIVE";
    }
    return "UNKNOWN";
}

// Only LIVE mode may place real exchange orders.
inline bool runtime_mode_is_live(RuntimeMode m) { return m == RuntimeMode::LIVE; }

// Parse a mode string (case-insensitive, whitespace-insensitive). false if bad.
inline bool parse_runtime_mode(const std::string& s, RuntimeMode& out) {
    std::string t; t.reserve(s.size());
    for (char c : s) if (!std::isspace((unsigned char)c)) t += (char)std::toupper((unsigned char)c);
    if (t == "DISABLED") { out = RuntimeMode::DISABLED; return true; }
    if (t == "SHADOW")   { out = RuntimeMode::SHADOW;   return true; }
    if (t == "PAPER")    { out = RuntimeMode::PAPER;    return true; }
    if (t == "LIVE")     { out = RuntimeMode::LIVE;     return true; }
    return false;
}

// Resolution result. ok=false => the caller MUST abort startup.
struct RuntimeModeResolution {
    RuntimeMode mode  = RuntimeMode::SHADOW;
    bool        ok    = true;
    std::string error;
};

// Resolve the single process mode from live_config.json's two mode-bearing
// signals and cross-check them for agreement.
//   have_mode_key : live_config.json carried an explicit "mode" string
//   mode_key      : its parsed value (ignored when have_mode_key is false)
//   cfg_shadow    : live_config.json's shadow_mode bool
// LIVE requires shadow_mode=false; every non-LIVE mode requires shadow_mode=true.
inline RuntimeModeResolution resolve_runtime_mode(bool cfg_shadow,
                                                  bool have_mode_key,
                                                  RuntimeMode mode_key) {
    RuntimeModeResolution r;
    if (have_mode_key) {
        bool key_is_live = runtime_mode_is_live(mode_key);
        if (key_is_live && cfg_shadow) {
            r.ok = false;
            r.error = "live_config.json: mode=LIVE but shadow_mode=true (contradiction)";
            return r;
        }
        if (!key_is_live && !cfg_shadow) {
            r.ok = false;
            r.error = std::string("live_config.json: mode=") + runtime_mode_str(mode_key)
                    + " but shadow_mode=false (contradiction)";
            return r;
        }
        r.mode = mode_key;
        return r;
    }
    r.mode = cfg_shadow ? RuntimeMode::SHADOW : RuntimeMode::LIVE;
    return r;
}

// Cross-check the resolved mode against a SECOND config file's shadow flag
// (binance_credentials.json). Any disagreement => abort.
//   resolved LIVE  requires creds shadow=false
//   resolved !LIVE requires creds shadow=true
inline RuntimeModeResolution cross_check_credentials_shadow(RuntimeMode resolved,
                                                            bool creds_shadow) {
    RuntimeModeResolution r; r.mode = resolved;
    bool resolved_live = runtime_mode_is_live(resolved);
    if (resolved_live && creds_shadow) {
        r.ok = false;
        r.error = "mode=LIVE but binance_credentials.json shadow_mode=true (config files disagree)";
    } else if (!resolved_live && !creds_shadow) {
        r.ok = false;
        r.error = std::string("mode=") + runtime_mode_str(resolved)
                + " but binance_credentials.json shadow_mode=false (config files disagree)";
    }
    return r;
}

} // namespace chimera
