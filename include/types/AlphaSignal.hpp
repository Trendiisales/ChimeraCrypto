#pragma once
#include <string>

namespace chimera {

struct AlphaSignal {
    std::string symbol;
    double score = 0.0;
    double confidence = 0.0;
    double size = 0.0;
};

}
