#pragma once
#include <cstdint>
#include <string>

namespace chimera {

enum SymbolId : uint8_t {
    SYM_BTC = 0,
    SYM_ETH = 1,
    SYM_SOL = 2,
    SYM_COUNT
};

inline SymbolId symbol_to_id(const std::string& s) {
    if (s == "btcusdt") return SYM_BTC;
    if (s == "ethusdt") return SYM_ETH;
    if (s == "solusdt") return SYM_SOL;
    return SYM_BTC;
}

}
