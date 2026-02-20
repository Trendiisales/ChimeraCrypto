#pragma once
#include <cstddef>

namespace chimera {

enum class SymbolID : std::size_t {
    ETH = 0,
    BTC = 1,
    SOL = 2,
    COUNT
};

inline constexpr const char* to_string(SymbolID id)
{
    switch (id)
    {
        case SymbolID::ETH: return "ethusdt";
        case SymbolID::BTC: return "btcusdt";
        case SymbolID::SOL: return "solusdt";
        default: return "";
    }
}

inline constexpr SymbolID from_string(const char* s)
{
    return
        (s[0]=='e') ? SymbolID::ETH :
        (s[0]=='b') ? SymbolID::BTC :
        SymbolID::SOL;
}

}
