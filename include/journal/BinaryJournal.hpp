#pragma once

#include <fstream>
#include <string>
#include "BinaryEvent.hpp"

namespace chimera {

class BinaryJournal {
public:
    explicit BinaryJournal(const std::string& filename);

    void append(uint64_t sequence,
                uint32_t type,
                const void* payload,
                uint32_t payload_size);

    void close();

private:
    std::ofstream file_;
};

}
