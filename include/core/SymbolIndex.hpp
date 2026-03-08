#pragma once
#include <cstdint>
#include <string>

namespace chimera {

// ============================================================================
// SymbolIndex — central registry for all traded symbols
//
// BTC is always id=0 (the lead-lag leader).
// All others are followers. Adding a new symbol = add entry here + main.cpp.
//
// LEADLAG followers: ETH, SOL, BNB, AVAX, LINK, POL
// ETH→SOL secondary lead-lag still uses ids 1 and 2.
// ============================================================================

static constexpr int MAX_SYMBOLS = 7;

// Short names (for logging)
static constexpr const char* SYM_SHORT[MAX_SYMBOLS] = {
    "BTC", "ETH", "SOL", "BNB", "AVAX", "LINK", "POL"
};

// Full Binance stream names (lowercase)
static constexpr const char* SYM_FULL[MAX_SYMBOLS] = {
    "btcusdt", "ethusdt", "solusdt", "bnbusdt", "avaxusdt", "linkusdt", "polusdt"
};

inline const char* sym_short(int id) {
    if (id >= 0 && id < MAX_SYMBOLS) return SYM_SHORT[id];
    return "???";
}

inline const char* sym_full(int id) {
    if (id >= 0 && id < MAX_SYMBOLS) return SYM_FULL[id];
    return "unknown";
}

inline int sym_id(const std::string& full_name) {
    for (int i = 0; i < MAX_SYMBOLS; ++i)
        if (full_name == SYM_FULL[i]) return i;
    return -1;
}

// Legacy enum kept for any code that uses it
enum SymbolId : uint8_t {
    SYM_BTC = 0,
    SYM_ETH = 1,
    SYM_SOL = 2,
    SYM_BNB = 3,
    SYM_AVAX = 4,
    SYM_LINK = 5,
    SYM_POL  = 6,
    SYM_COUNT = MAX_SYMBOLS
};

// Legacy function kept for compatibility
inline SymbolId symbol_to_id(const std::string& s) {
    int id = sym_id(s);
    return (id >= 0) ? static_cast<SymbolId>(id) : SYM_BTC;
}

} // namespace chimera
