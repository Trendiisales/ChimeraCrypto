#pragma once
#include <unordered_map>
#include <string>

namespace chimera {

class Portfolio {
public:
    void apply_fill(const std::string& symbol,
                    double price,
                    double qty);

    double equity() const;

    double position(const std::string& symbol) const;

private:
    std::unordered_map<std::string, double> positions_;
    double cash_ = 0.0;
};

}
