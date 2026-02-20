#pragma once
#include <vector>
#include <algorithm>
#include <cstdint>

namespace chimera {

struct DepthUpdate {
    int64_t U;
    int64_t u;
    std::vector<std::pair<double,double>> bids;
    std::vector<std::pair<double,double>> asks;
};

struct Level {
    double price;
    double size;
};

class L2SyncBook {
public:
    L2SyncBook() : last_update_id_(0), synced_(false) {
        bids_.reserve(200);
        asks_.reserve(200);
    }

    void load_snapshot(int64_t lastUpdateId, const std::vector<Level>& bids, const std::vector<Level>& asks) {
        last_update_id_ = lastUpdateId;
        bids_ = bids;
        asks_ = asks;
        synced_ = true;
    }

    bool apply_diff(const DepthUpdate& diff) {
        if (!synced_) return false;
        if (diff.u < last_update_id_) return false;
        if (diff.U > last_update_id_ + 1) return false;

        for (const auto& b : diff.bids)
            update_side(bids_, b.first, b.second, true);

        for (const auto& a : diff.asks)
            update_side(asks_, a.first, a.second, false);

        last_update_id_ = diff.u;
        return true;
    }

    double best_bid() const {
        if (bids_.empty()) return 0.0;
        return bids_.front().price;
    }

    double best_ask() const {
        if (asks_.empty()) return 0.0;
        return asks_.front().price;
    }

    double imbalance_top(int depth=5) const {
        double b=0,a=0;
        for(int i=0;i<depth && i<(int)bids_.size();++i) b+=bids_[i].size;
        for(int i=0;i<depth && i<(int)asks_.size();++i) a+=asks_[i].size;
        if(b+a==0) return 0.5;
        return b/(b+a);
    }

private:
    void update_side(std::vector<Level>& side, double price, double size, bool bid) {
        auto it = std::find_if(side.begin(), side.end(), [&](const Level& l){ return l.price==price; });

        if(size==0){
            if(it!=side.end()) side.erase(it);
        } else {
            if(it!=side.end()){
                it->size=size;
            } else {
                side.push_back({price,size});
            }
        }

        std::sort(side.begin(), side.end(), [bid](const Level&a,const Level&b){
            return bid ? a.price>b.price : a.price<b.price;
        });
    }

private:
    int64_t last_update_id_;
    bool synced_;
    std::vector<Level> bids_;
    std::vector<Level> asks_;
};

}
