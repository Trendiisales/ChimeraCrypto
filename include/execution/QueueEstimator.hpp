#pragma once

namespace chimera {

class QueueEstimator {
public:
    double estimate_fill_prob(double size,
                              double book_liquidity) const
    {
        if (book_liquidity == 0)
            return 0;

        double ratio = size / book_liquidity;
        return 1.0 - ratio;
    }
};

}
