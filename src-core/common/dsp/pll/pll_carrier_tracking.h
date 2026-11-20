#pragma once

#include "common/dsp/block.h"

/*
PLL syncing against a center carrier, and bringing it to DC
*/
namespace dsp
{
    class PLLCarrierTrackingBlock : public Block<complex_t, complex_t>
    {
    private:
        float d_max_freq, d_min_freq;
        float d_damping, d_loop_bw;
        float d_alpha, d_beta;
        float d_phase, d_freq;
        int d_acquire_samples = 0, d_acquire_count = 0;
        double d_acquire_re = 0, d_acquire_im = 0, d_acquire_power = 0;

        void work();

    public:
        PLLCarrierTrackingBlock(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw, float max, float min);

        float getFreq() { return d_freq; }
        // Enable coarse acquisition before start(); disabled by default.
        void acquireFrequency(int samples) { d_acquire_samples = samples > 1 ? samples : 2; }
    };
}
