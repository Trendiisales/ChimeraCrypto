#pragma once
namespace chimera {

class ExecutionQuality {
public:
    void record_fill(double slippage);
    double avg_slippage() const;

private:
    double total_slippage_ = 0.0;
    int fill_count_ = 0;
};

}
