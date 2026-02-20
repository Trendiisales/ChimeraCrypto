#pragma once
#include "Journal.hpp"
#include "spine/EventSpine.hpp"

namespace chimera {

class ReplayEngine {
public:
    ReplayEngine(EventSpine& spine,
                 const Journal& journal);

    void replay();

private:
    EventSpine& spine_;
    const Journal& journal_;
};

}
