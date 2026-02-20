#include "spine/Journal.hpp"

namespace chimera {

void Journal::append(const Event& ev)
{
    log_.push_back(ev);
}

const std::vector<Event>& Journal::events() const
{
    return log_;
}

}
