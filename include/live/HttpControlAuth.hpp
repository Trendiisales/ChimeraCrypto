#pragma once
// ============================================================================
// HttpControlAuth — auth/method helpers for the :8080 control API.
//
// Phase-1 review fix (2026-07-11): the control server bound to INADDR_ANY (the
// public IP) and exposed POST /api/kill, /api/session_reset, /api/ratchet_reset,
// /api/daily_kill_clear with NO authentication. These helpers add a shared-token
// check + strict POST-only enforcement for the mutating endpoints. Extracted to
// a header so the logic is unit-testable without spinning up the socket server.
// ============================================================================
#include <string>
#include <cstring>
#include <cctype>

namespace chimera {

// True iff the raw HTTP request line begins with "POST ".
inline bool http_request_is_post(const char* req) {
    return req && std::strncmp(req, "POST ", 5) == 0;
}

// Case-insensitive substring search (portable; avoids strcasestr).
inline const char* http_ci_find(const char* hay, const char* needle) {
    if (!hay || !needle) return nullptr;
    size_t nl = std::strlen(needle);
    if (nl == 0) return hay;
    for (const char* p = hay; *p; ++p) {
        size_t i = 0;
        while (i < nl && p[i] &&
               std::tolower((unsigned char)p[i]) == std::tolower((unsigned char)needle[i])) ++i;
        if (i == nl) return p;
    }
    return nullptr;
}

// Value that follows `key` (e.g. "X-Auth-Token:" or "token="), up to the next
// whitespace / CR / LF / '&' / ';'. Empty string if `key` is absent.
inline std::string http_extract_after(const char* req, const char* key) {
    const char* p = http_ci_find(req, key);
    if (!p) return "";
    p += std::strlen(key);
    while (*p == ' ' || *p == '\t') ++p;
    std::string v;
    while (*p && *p != ' ' && *p != '\r' && *p != '\n' && *p != '&' && *p != ';') v += *p++;
    return v;
}

// Authorised iff a non-empty server token is configured AND the request carries
// it via the X-Auth-Token header or a ?token=/&token= query parameter.
// No token configured => always DENY (fail closed).
inline bool http_control_authorized(const char* req, const std::string& server_token) {
    if (server_token.empty()) return false;
    if (http_extract_after(req, "X-Auth-Token:") == server_token && !server_token.empty()) return true;
    if (http_extract_after(req, "token=") == server_token && !server_token.empty()) return true;
    return false;
}

// True iff the request targets one of the state-mutating control endpoints.
inline bool http_is_mutating_control(const char* req) {
    return std::strstr(req, "/api/kill")
        || std::strstr(req, "/api/ratchet_reset")
        || std::strstr(req, "/api/daily_kill_clear")
        || std::strstr(req, "/api/session_reset");
}

} // namespace chimera
