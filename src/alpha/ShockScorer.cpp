#include "alpha/ShockScorer.hpp"

namespace chimera {

AlphaSignal ShockScorer::score(const AlphaSignal& in)
{
    AlphaSignal out = in;
    out.confidence = in.score > 0 ? 0.8 : 0.2;
    return out;
}

}
