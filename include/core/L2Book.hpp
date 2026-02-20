#pragma once
#include <vector>
#include <algorithm>
#include <cstdint>

namespace chimera {

struct Level {
    double price;
    double size;
};

class L2Book {
public:
    L2Book() {
        bids_.reserve(50);
        asks_.reserve(50);
    }

    void update_bid(double price, double size) {
        update_side(bids_, price, size, true);
    }

    void update_ask(double price, double size) {
        update_side(asks_, price, size, false);
    }

    double best_bid() const {
        if (bids_.empty()) return 0.0;
        return bids_.front().price;
    }

    double best_ask() const {
        if (asks_.empty()) return 0.0;
        return asks_.front().price;
    }

    double bid_volume_top(int levels = 5) const {
        double sum = 0.0;
        for (int i = 0; i < levels && i < (int)bids_.size(); ++i)
            sum += bids_[i].size;
        return sum;
    }

    double ask_volume_top(int levels = 5) const {
        double sum = 0.0;
        for (int i = 0; i < levels && i < (int)asks_.size(); ++i)
            sum += asks_[i].size;
        return sum;
    }

    double imbalance_top(int levels = 5) const {
        double b = bid_volume_top(levels);
        double a = ask_volume_top(levels);
        if (b + a == 0.0) return 0.5;
        return b / (b + a);
    }

private:
    void update_side(std::vector<Level>& side, double price, double size, bool bid) {
        auto it = std::find_if(side.begin(), side.end(), [&](const Level& l) { return l.price == price; });

        if (size == 0.0) {
            if (it != side.end()) side.erase(it);
            return;
        }

        if (it != side.end()) {
            it->size = size;
        } else {
            side.push_back({price, size});
        }

        std::sort(side.begin(), side.end(), [bid](const Level& a, const Level& b) {
            return bid ? a.price > b.price : a.price < b.price;
        });
    }

private:
    std::vector<Level> bids_;
    std::vector<Level> asks_;
};

}
