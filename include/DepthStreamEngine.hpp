#pragma once
#include "DepthBook.hpp"
#include "RestThrottle.hpp"
#include "WebSocketReconnectController.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

namespace chimera {

class DepthStreamEngine {
public:
    using SnapshotRequestCallback = std::function<void(const std::string&)>;
    
    DepthStreamEngine() = default;
    
    void setSnapshotCallback(SnapshotRequestCallback cb) {
        snapshotCallback_ = cb;
    }
    
    void registerSymbol(const std::string& symbol) {
        auto book = std::make_unique<DepthBook>(symbol);
        
        // Set resync callback to trigger snapshot
        book->setResyncCallback([this](const std::string& sym) {
            if (snapshotCallback_) {
                snapshotCallback_(sym);
            }
        });
        
        books_[symbol] = std::move(book);
    }
    
    void onSnapshot(const std::string& symbol, const Snapshot& snap) {
        auto it = books_.find(symbol);
        if (it == books_.end()) {
            return;
        }
        
        it->second->applySnapshot(snap);
    }
    
    void onDiff(const std::string& symbol, const DiffEvent& diff) {
        auto it = books_.find(symbol);
        if (it == books_.end()) {
            return;
        }
        
        it->second->applyDiff(diff);
    }
    
    void onWsDisconnect(const std::string& symbol) {
        reconnect_.notifyDisconnect();
        reconnect_.backoff();
        
        auto it = books_.find(symbol);
        if (it != books_.end()) {
            it->second->markWsReconnect();
        }
    }
    
    void onWsConnect() {
        reconnect_.reset();
    }
    
    DepthBook* book(const std::string& symbol) {
        auto it = books_.find(symbol);
        return (it != books_.end()) ? it->second.get() : nullptr;
    }
    
    RestThrottle& throttle() {
        return throttle_;
    }
    
private:
    std::unordered_map<std::string, std::unique_ptr<DepthBook>> books_;
    WebSocketReconnectController reconnect_;
    RestThrottle throttle_;
    SnapshotRequestCallback snapshotCallback_;
};

} // namespace chimera
