#pragma once
#include <string>

namespace chimera {

class SpotExecutor {
public:
    SpotExecutor(bool shadow);

    void execute(const std::string& symbol,
                 bool is_buy,
                 double qty,
                 double price);

private:
    bool shadow_;
};

}
