#pragma once
#include <cstddef>

namespace chimera {

class DeskServer {
public:
    void start();
    void push(const char* data,
              std::size_t len);
};

}
