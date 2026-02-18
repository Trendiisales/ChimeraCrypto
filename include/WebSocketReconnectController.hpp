#pragma once
#include <chrono>
#include <thread>
#include <atomic>

namespace chimera {

class WebSocketReconnectController {
public:
    void notifyDisconnect() {
        attempts_++;
    }
    
    void backoff() {
        int delay = 100 * attempts_;
        if (delay > 3000) {
            delay = 3000;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    
    void reset() {
        attempts_ = 0;
    }
    
    int getAttempts() const {
        return attempts_.load();
    }
    
private:
    std::atomic<int> attempts_{0};
};

} // namespace chimera
