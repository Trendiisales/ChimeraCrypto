#pragma once
#include <array>
#include <atomic>
#include <string>

namespace chimera {

struct EngineSnapshot
{
    double equity = 0.0;
    double unrealized = 0.0;
    double total_exposure = 0.0;
    uint64_t tick_count = 0;

    struct SymbolView {
        char symbol[16];
        double last_mid = 0.0;
        double position = 0.0;
        double volatility = 0.0;
    };

    std::array<SymbolView, 16> symbols;
    size_t symbol_count = 0;
};

}
