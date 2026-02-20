#pragma once
#include <algorithm>
#include <cstdint>

namespace chimera {

class ExchangeLatencyEngine {
public:
    void record(int64_t receive_ms, int64_t exchange_ms)
    {
        int64_t raw = receive_ms - exchange_ms;

        if (!offset_locked_) {
            offset_accum_ += raw;
            offset_samples_++;
            if (offset_samples_ >= calibration_samples_) {
                clock_offset_ms_ = offset_accum_ / offset_samples_;
                offset_locked_ = true;
            }
            return;
        }

        int64_t corrected = raw - clock_offset_ms_;
        if (corrected < 0) corrected = 0;

        buffer_[write_idx_] = (double)corrected;
        write_idx_ = (write_idx_ + 1) % max_samples_;
        if (size_ < max_samples_) size_++;
    }

    bool ready() const { return offset_locked_; }

    double mean() const {
        if (size_ == 0) return 0.0;
        double s = 0.0;
        for (size_t i = 0; i < size_; ++i)
            s += buffer_[i];
        return s / (double)size_;
    }

    double min() const {
        if (size_ == 0) return 0.0;
        double m = buffer_[0];
        for (size_t i = 1; i < size_; ++i)
            if (buffer_[i] < m) m = buffer_[i];
        return m;
    }

    double max() const {
        if (size_ == 0) return 0.0;
        double m = buffer_[0];
        for (size_t i = 1; i < size_; ++i)
            if (buffer_[i] > m) m = buffer_[i];
        return m;
    }

    double p50() const { return percentile(0.50); }
    double p95() const { return percentile(0.95); }

    double latest() const {
        if (size_ == 0) return 0.0;
        size_t idx = (write_idx_ == 0) ? max_samples_ - 1 : write_idx_ - 1;
        return buffer_[idx];
    }

private:
    double percentile(double p) const {
        if (size_ == 0) return 0.0;

        double temp[max_samples_];
        for (size_t i = 0; i < size_; ++i)
            temp[i] = buffer_[i];

        std::sort(temp, temp + size_);
        size_t idx = (size_t)(p * (size_ - 1));
        return temp[idx];
    }

    static constexpr int calibration_samples_ = 100;
    static constexpr size_t max_samples_ = 2048;

    bool offset_locked_ = false;
    int64_t clock_offset_ms_ = 0;
    int64_t offset_accum_ = 0;
    int offset_samples_ = 0;

    double buffer_[max_samples_];
    size_t write_idx_ = 0;
    size_t size_ = 0;
};

}
