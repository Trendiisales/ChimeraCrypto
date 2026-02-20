#include "spine/EventSpine.hpp"

namespace chimera {

void EventSpine::publish(const Event& ev)
{
    for (auto& h : handlers_)
        h(ev);
}

void EventSpine::subscribe(Handler handler)
{
    handlers_.push_back(handler);
}

}
