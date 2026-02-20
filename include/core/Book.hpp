#pragma once
#include <vector>

namespace chimera {

struct Book {
    double bid = 0.0;
    double ask = 0.0;

    double mid() const {
        return (bid + ask) * 0.5;
    }
};

}
