#pragma once
#include <fstream>
#include <functional>
#include "BinaryEvent.hpp"

namespace chimera {

class BinaryReplay {
public:
    explicit BinaryReplay(const std::string& filename);

    void replay(std::function<void(const EventHeader&,
                                   const std::vector<char>&)> handler);

private:
    std::ifstream file_;
};

}
