#pragma once
#include <cstdint>

namespace chimera {

struct ExchangeLatencyBreakdown {
    double networkMs = 0.0;
    double exchangeProcessingMs = 0.0;
    double localProcessingMs = 0.0;
};

class ExchangeTimeReconciler {
public:
    ExchangeLatencyBreakdown compute(uint64_t sendTs,
                                    uint64_t exchangeTs,
                                    uint64_t recvTs) {
        ExchangeLatencyBreakdown out;

        double total = (recvTs - sendTs) / 1000.0;
        double exchangeToRecv = (recvTs - exchangeTs) / 1000.0;

        out.exchangeProcessingMs = (exchangeTs - sendTs) / 1000.0;
        out.networkMs = exchangeToRecv;
        out.localProcessingMs = total - out.exchangeProcessingMs - out.networkMs;

        return out;
    }
};

} // namespace chimera
