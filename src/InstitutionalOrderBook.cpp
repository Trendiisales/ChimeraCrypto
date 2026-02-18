#include "InstitutionalOrderBook.hpp"
#include <cstdio>
#include <cmath>

namespace chimera {

void InstitutionalOrderBook::hardReset() {
    std::lock_guard<std::mutex> lock(mtx_);
    bids_.clear();
    asks_.clear();
    lastUpdateId_ = 0;
    state_ = State::REBUILDING;
    printf("[BOOK] HARD RESET\n");
}

void InstitutionalOrderBook::loadSnapshot(
    uint64_t lastUpdateId,
    const std::vector<Level>& bids,
    const std::vector<Level>& asks)
{
    std::lock_guard<std::mutex> lock(mtx_);

    bids_.clear();
    asks_.clear();

    for (const auto& l : bids)
        if (l.size > 0.0)
            bids_[l.price] = l.size;

    for (const auto& l : asks)
        if (l.size > 0.0)
            asks_[l.price] = l.size;

    lastUpdateId_ = lastUpdateId;

    if (!bids_.empty() && !asks_.empty())
        state_ = State::LIVE;
    else
        state_ = State::REBUILDING;

    lastUpdateTime_ = std::chrono::steady_clock::now();

    printf("[SNAPSHOT] Applied: bids=%zu asks=%zu state=%s lastUpdateId=%lu\n",
           bids_.size(),
           asks_.size(),
           state_ == State::LIVE ? "LIVE" : "REBUILDING",
           lastUpdateId_);
}

bool InstitutionalOrderBook::applyIncremental(
    uint64_t firstId,
    uint64_t finalId,
    const std::vector<Level>& bids,
    const std::vector<Level>& asks)
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (state_ != State::LIVE)
        return false;

    // Binance-compliant sequence handling
    // Ignore old updates
    if (finalId <= lastUpdateId_)
        return true;

    // REAL GAP detection per Binance spec
    if (firstId > lastUpdateId_ + 1) {
        printf("[BOOK_ERROR] REAL GAP detected. Expected >= %lu got %lu\n",
               lastUpdateId_ + 1, firstId);

        bids_.clear();
        asks_.clear();
        lastUpdateId_ = 0;
        state_ = State::REBUILDING;
        return false;
    }

    // Accept overlapping updates where U <= lastUpdateId+1 <= u
    for (const auto& l : bids) {
        if (l.size == 0.0)
            bids_.erase(l.price);
        else
            bids_[l.price] = l.size;
    }

    for (const auto& l : asks) {
        if (l.size == 0.0)
            asks_.erase(l.price);
        else
            asks_[l.price] = l.size;
    }

    // Safety check for crossed book
    if (!bids_.empty() && !asks_.empty()) {
        double bid = bids_.begin()->first;
        double ask = asks_.begin()->first;
        
        if (bid >= ask) {
            printf("[BOOK_ERROR] Crossed book detected (bid=%.2f >= ask=%.2f) — forcing reset\n", bid, ask);
            bids_.clear();
            asks_.clear();
            lastUpdateId_ = 0;
            state_ = State::REBUILDING;
            return false;
        }
    }

    lastUpdateId_ = finalId;
    lastUpdateTime_ = std::chrono::steady_clock::now();

    return true;
}

bool InstitutionalOrderBook::ready() const {
    std::lock_guard<std::mutex> lock(mtx_);

    if (state_ != State::LIVE)
        return false;

    if (bids_.empty() || asks_.empty())
        return false;

    double bid = bids_.begin()->first;
    double ask = asks_.begin()->first;

    if (bid <= 0.0 || ask <= 0.0)
        return false;

    if (bid >= ask)
        return false;

    return true;
}

bool InstitutionalOrderBook::stale() const {
    std::lock_guard<std::mutex> lock(mtx_);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - lastUpdateTime_).count();

    return elapsed > 5;
}

double InstitutionalOrderBook::bestBid() const {
    if (!ready()) return 0.0;
    return bids_.begin()->first;
}

double InstitutionalOrderBook::bestAsk() const {
    if (!ready()) return 0.0;
    return asks_.begin()->first;
}

double InstitutionalOrderBook::mid() const {
    if (!ready()) return 0.0;
    return (bestBid() + bestAsk()) * 0.5;
}

double InstitutionalOrderBook::imbalance() const {
    if (!ready()) return 0.0;

    double bidVol = 0.0;
    double askVol = 0.0;

    int depth = 5;
    int i = 0;

    for (const auto& b : bids_) {
        bidVol += b.second;
        if (++i >= depth) break;
    }

    i = 0;
    for (const auto& a : asks_) {
        askVol += a.second;
        if (++i >= depth) break;
    }

    if (bidVol + askVol == 0.0)
        return 0.0;

    return (bidVol - askVol) / (bidVol + askVol);
}

InstitutionalOrderBook::State InstitutionalOrderBook::state() const {
    return state_;
}

}
