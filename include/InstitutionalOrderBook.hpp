#pragma once
#include "types.hpp"
#include <map>
#include <vector>
#include <mutex>
#include <chrono>

namespace chimera {

class InstitutionalOrderBook {
public:
    enum class State {
        EMPTY,
        REBUILDING,
        LIVE
    };

    void hardReset();
    void loadSnapshot(uint64_t lastUpdateId,
                      const std::vector<Level>& bids,
                      const std::vector<Level>& asks);

    bool applyIncremental(uint64_t firstId,
                          uint64_t finalId,
                          const std::vector<Level>& bids,
                          const std::vector<Level>& asks);

    bool ready() const;
    bool stale() const;

    double mid() const;
    double bestBid() const;
    double bestAsk() const;
    double imbalance() const;

    State state() const;

private:
    mutable std::mutex mtx_;

    std::map<double,double,std::greater<double>> bids_;
    std::map<double,double,std::less<double>> asks_;

    uint64_t lastUpdateId_ = 0;
    State state_ = State::EMPTY;

    std::chrono::steady_clock::time_point lastUpdateTime_;
};

}
