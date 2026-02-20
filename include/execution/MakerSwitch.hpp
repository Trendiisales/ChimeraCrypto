#pragma once

namespace chimera {

class MakerSwitch {
public:
    bool use_maker(double edge_bps) const
    {
        return edge_bps < 12.0;
    }
};

}
