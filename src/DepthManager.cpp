#include "DepthManager.hpp"
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <cstdio>

namespace chimera {

DepthManager::DepthManager(const std::string& symbol) 
    : symbol_(symbol), bootstrapper_(symbol) {
    
    // Set up snapshot request callback
    bootstrapper_.set_snapshot_request_callback([this](const std::string& sym) {
        request_snapshot(sym);
    });
    
    // Start background snapshot thread
    snapshot_thread_ = std::thread(&DepthManager::snapshot_thread_func, this);
}

DepthManager::~DepthManager() {
    running_.store(false);
    if (snapshot_thread_.joinable()) {
        snapshot_thread_.join();
    }
}

void DepthManager::request_snapshot(const std::string& symbol) {
    snapshot_pending_.store(true);
}

void DepthManager::snapshot_thread_func() {
    while (running_.load()) {
        if (snapshot_pending_.load()) {
            snapshot_pending_.store(false);
            
            Snapshot snap;
            if (SnapshotFetcher::fetch(symbol_, snap)) {
                std::unique_lock lock(mtx_);
                bootstrapper_.on_snapshot(snap);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void DepthManager::handleWsDepth(uint64_t U, uint64_t u, 
                                 const std::vector<Level>& bids, 
                                 const std::vector<Level>& asks) {
    // Convert to L2Level format
    std::vector<L2Level> l2_bids, l2_asks;
    for (const auto& b : bids) {
        l2_bids.push_back({b.price, b.size});
    }
    for (const auto& a : asks) {
        l2_asks.push_back({a.price, a.size});
    }
    
    DepthEvent ev;
    ev.symbol = symbol_;
    ev.U = U;
    ev.u = u;
    ev.bids = l2_bids;
    ev.asks = l2_asks;
    
    std::unique_lock lock(mtx_);
    bootstrapper_.on_ws_delta(ev);
}

DepthManager::BookView DepthManager::book() const {
    std::shared_lock lock(mtx_);
    
    BookView view{};
    view.ready = bootstrapper_.ready();
    
    if (!view.ready) {
        view.mid = 0.0;
        view.bestBid = 0.0;
        view.bestAsk = 0.0;
        view.imbalance = 0.5;
        return view;
    }
    
    const auto& l2book = bootstrapper_.book();
    
    view.bestBid = l2book.best_bid();
    view.bestAsk = l2book.best_ask();
    view.mid = l2book.mid();
    view.imbalance = l2book.imbalance_top5();
    
    return view;
}

std::array<Level, Config::BOOK_DEPTH> DepthManager::bid_array() const {
    std::shared_lock lock(mtx_);
    std::array<Level, Config::BOOK_DEPTH> arr{};
    
    if (!bootstrapper_.ready())
        return arr;
    
    // This would need L2Book to expose level iteration
    // For now, return empty - can enhance later
    
    return arr;
}

std::array<Level, Config::BOOK_DEPTH> DepthManager::ask_array() const {
    std::shared_lock lock(mtx_);
    std::array<Level, Config::BOOK_DEPTH> arr{};
    
    if (!bootstrapper_.ready())
        return arr;
    
    // This would need L2Book to expose level iteration
    // For now, return empty - can enhance later
    
    return arr;
}

}
