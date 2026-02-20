#pragma once
#include <vector>
#include "Event.hpp"

namespace chimera {

class Journal {
public:
    void append(const Event& ev);
    const std::vector<Event>& events() const;

private:
    std::vector<Event> log_;
};

}
