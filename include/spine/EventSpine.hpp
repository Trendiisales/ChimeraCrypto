#pragma once
#include <vector>
#include <functional>
#include "Event.hpp"

namespace chimera {

class EventSpine {
public:
    using Handler = std::function<void(const Event&)>;

    void publish(const Event& ev);
    void subscribe(Handler handler);

private:
    std::vector<Handler> handlers_;
};

}
