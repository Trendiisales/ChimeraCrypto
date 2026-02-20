#pragma once

namespace chimera {

class ExecutionPolicy {
public:
    void update_latency(double rtt_ms);
    void update_spread(double spread);
    void record_reject();

    bool allow_trade() const;

private:
    double last_rtt_ = 0.0;
    double last_spread_ = 0.0;
    int reject_count_ = 0;

    double max_latency_ms_ = 25.0;
    double max_spread_ = 50.0;
    int max_rejects_ = 5;
};

}
