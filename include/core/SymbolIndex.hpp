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
// Session 30: Added PEPE, WIF, FET, ONDO, TIA (5 new meme/AI/RWA symbols)
// Session 35 (AUDIT-2026-S35): Added HBAR, INJ, ADA, TRX, SEI (5 L1/DeFi)
// ============================================================================

static constexpr int MAX_SYMBOLS = 46;

// Short names (for logging)
static constexpr const char* SYM_SHORT[MAX_SYMBOLS] = {
    "BTC", "ETH", "SOL", "BNB", "AVAX", "LINK", "XRP", "DOGE",
    "SUI", "APT", "NEAR", "ARB",
    "PEPE", "WIF", "FET", "ONDO", "TIA",
    "HBAR", "INJ", "ADA", "TRX", "SEI",
    "OP", "MATIC", "ATOM", "FIL", "AAVE", "UNI", "LDO", "ENA", "JUP",
    "TON", "DOT", "ICP", "RENDER", "PYTH", "GRT", "SAND", "MANA",
    "CRV", "COMP", "MKR", "IMX", "STX", "ARKM", "MASK"
};

// Full Binance stream names (lowercase)
static constexpr const char* SYM_FULL[MAX_SYMBOLS] = {
    "btcusdt", "ethusdt", "solusdt", "bnbusdt", "avaxusdt", "linkusdt", "xrpusdt", "dogeusdt",
    "suiusdt", "aptusdt", "nearusdt", "arbusdt",
    "pepeusdt", "wifusdt", "fetusdt", "ondousdt", "tiausdt",
    "hbarusdt", "injusdt", "adausdt", "trxusdt", "seiusdt",
    "opusdt", "maticusdt", "atomusdt", "filusdt", "aaveusdt", "uniusdt",
    "ldousdt", "enausdt", "jupusdt",
    "tonusdt", "dotusdt", "icpusdt", "renderusdt", "pythusdt", "grtusdt", "sandusdt", "manausdt",
    "crvusdt", "compusdt", "mkrusdt", "imxusdt", "stxusdt", "arkmusdt", "maskusdt"
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
    SYM_BTC  = 0,
    SYM_ETH  = 1,
    SYM_SOL  = 2,
    SYM_BNB  = 3,
    SYM_AVAX = 4,
    SYM_LINK = 5,
    SYM_XRP  = 6,
    SYM_DOGE = 7,
    SYM_SUI  = 8,
    SYM_APT  = 9,
    SYM_NEAR = 10,
    SYM_ARB  = 11,
    SYM_PEPE = 12,
    SYM_WIF  = 13,
    SYM_FET  = 14,
    SYM_ONDO = 15,
    SYM_TIA  = 16,
    SYM_HBAR = 17,
    SYM_INJ  = 18,
    SYM_ADA  = 19,
    SYM_TRX  = 20,
    SYM_SEI  = 21,
    SYM_OP    = 22,
    SYM_MATIC = 23,
    SYM_ATOM  = 24,
    SYM_FIL   = 25,
    SYM_AAVE  = 26,
    SYM_UNI   = 27,
    SYM_LDO   = 28,
    SYM_ENA   = 29,
    SYM_JUP   = 30,
    SYM_TON   = 31,
    SYM_DOT   = 32,
    SYM_ICP   = 33,
    SYM_RENDER = 34,
    SYM_PYTH  = 35,
    SYM_GRT   = 36,
    SYM_SAND  = 37,
    SYM_MANA  = 38,
    SYM_CRV   = 39,
    SYM_COMP  = 40,
    SYM_MKR   = 41,
    SYM_IMX   = 42,
    SYM_STX   = 43,
    SYM_ARKM  = 44,
    SYM_MASK  = 45,
    SYM_COUNT = MAX_SYMBOLS
};

// Legacy function kept for compatibility
inline SymbolId symbol_to_id(const std::string& s) {
    int id = sym_id(s);
    return (id >= 0) ? static_cast<SymbolId>(id) : SYM_BTC;
}

} // namespace chimera
