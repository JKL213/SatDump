#pragma once

#include "common/dsp/block.h"

namespace dsp
{

    // Removes DC using an exponential moving average, independently for I/Q.

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
        CorrectIQBlock(std::shared_ptr<dsp::stream<T>> input, float alpha = 0.0001);
        ~CorrectIQBlock();
    };
} // namespace dsp
