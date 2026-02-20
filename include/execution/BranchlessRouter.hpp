#pragma once
#include <array>

namespace chimera {

struct VenueScore {
    double latency;
    double fill_rate;
    double cost;
    
    inline double total() const {
        return fill_rate * 100.0 - latency * 0.5 - cost * 2.0;
    }
};

template<int N>
class BranchlessRouter {
public:
    inline int best_venue(const std::array<VenueScore, N>& venues) const {
        double scores[N];
        for (int i = 0; i < N; ++i)
            scores[i] = venues[i].total();
        
        int best = 0;
        for (int i = 1; i < N; ++i)
            best += (scores[i] > scores[best]) * (i - best);
        
        return best;
    }
};

}
