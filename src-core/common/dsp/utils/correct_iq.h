#pragma once

#include "common/dsp/block.h"
#include <type_traits>

namespace dsp
{

    // Removes DC using an exponential moving average, independently for I/Q.
    template <typename T>
    inline T correct_iq_sample(T sample, double alpha, double &acc_i, double &acc_q)
    {
        if constexpr (std::is_same_v<T, complex_t>)
        {
            acc_i += alpha * (sample.real - acc_i);
            acc_q += alpha * (sample.imag - acc_q);
            return complex_t(sample.real - acc_i, sample.imag - acc_q);
        }
        else
        {
            acc_i += alpha * (sample - acc_i);
            return sample - acc_i;
        }
    }

    template <typename T>
    class CorrectIQBlock : public Block<T, T>
    {
    private:
        double alpha;
        // Double precision prevents the estimator from stalling before the DC
        // is removed at MHz sample rates (small alpha).
        double acc_i = 0;
        double acc_q = 0;

        void work();

    public:
        static constexpr double DEFAULT_TIME_CONSTANT_SECONDS = 1.0 / 50.0;
        static double alpha_from_samplerate(double samplerate, double time_constant_seconds = DEFAULT_TIME_CONSTANT_SECONDS);

        CorrectIQBlock(std::shared_ptr<dsp::stream<T>> input, double alpha = 0.0001);
        ~CorrectIQBlock();
    };
} // namespace dsp
