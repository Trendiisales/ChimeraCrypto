#include "gui/DeskServer.hpp"
#include <iostream>

namespace chimera {

void DeskServer::start() {}

void DeskServer::push(const char* data,
                      std::size_t len)
{
    std::cout.write(data, len);
    std::cout << "\n";
}

}
