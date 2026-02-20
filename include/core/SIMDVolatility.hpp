#pragma once
#include <immintrin.h>
#include <cstddef>

namespace chimera {

class SIMDVolatility {
public:
    SIMDVolatility() : idx_(0) {
        for (int i = 0; i < 256; ++i)
            buffer_[i] = 0.0;
    }

    inline void push(double v) {
        buffer_[idx_++] = v;
        if (idx_ >= 256) idx_ = 0;
    }

    inline double mean_abs() const {
        __m256d sum = _mm256_setzero_pd();

        for (int i = 0; i < 256; i += 4) {
            __m256d x = _mm256_loadu_pd(&buffer_[i]);
            __m256d absx = _mm256_andnot_pd(_mm256_set1_pd(-0.0), x);
            sum = _mm256_add_pd(sum, absx);
        }

        double tmp[4];
        _mm256_storeu_pd(tmp, sum);
        return (tmp[0] + tmp[1] + tmp[2] + tmp[3]) / 256.0;
    }

private:
    double buffer_[256];
    size_t idx_;
};

}
