#include <iostream>
#include <thread>
#include <chrono>

#include "InstitutionalEngine.hpp"

int main()
{
    chimera::InstitutionalEngine engine(10000.0);

    for (int i = 0; i < 5; ++i)
    {
        engine.tick(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Final Equity: $" << engine.get_equity() << std::endl;
    return 0;
}
