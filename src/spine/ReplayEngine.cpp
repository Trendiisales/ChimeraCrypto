#include "spine/ReplayEngine.hpp"

namespace chimera {

ReplayEngine::ReplayEngine(EventSpine& spine,
                           const Journal& journal)
    : spine_(spine),
      journal_(journal)
{}

void ReplayEngine::replay()
{
    for (const auto& ev : journal_.events())
        spine_.publish(ev);
}

}
