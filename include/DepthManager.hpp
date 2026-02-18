#pragma once
#include "types.hpp"
#include "config.hpp"
#include "l2/L2Bootstrapper.hpp"
#include "l2/SnapshotFetcher.hpp"
#include <functional>
#include <map>
#include <shared_mutex>
#include <deque>
#include <string>
#include <vector>
#include <array>
#include <thread>
#include <atomic>

namespace chimera {

class DepthManager {
public:
    struct BookView {
        double mid;
        double bestBid;
        double bestAsk;
        double imbalance;
        bool ready;
    };

    explicit DepthManager(const std::string& symbol);
    ~DepthManager();
    
    void handleWsDepth(uint64_t U, uint64_t u, const std::vector<Level>& bids, const std::vector<Level>& asks);
    
    BookView book() const;
    std::array<Level, Config::BOOK_DEPTH> bid_array() const;
    std::array<Level, Config::BOOK_DEPTH> ask_array() const;

private:
    void snapshot_thread_func();
    void request_snapshot(const std::string& symbol);

    std::string symbol_;
    
    L2Bootstrapper bootstrapper_;
    
    mutable std::shared_mutex mtx_;
    
    std::thread snapshot_thread_;
    std::atomic<bool> running_{true};
    std::atomic<bool> snapshot_pending_{false};
};

}
