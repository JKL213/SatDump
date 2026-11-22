#pragma once

/**
 * @file correct_iq.h
 */

#include "dsp/block_simple.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        /**
         * @brief Simple Correct IQ block (removes a DC spike).
         *
         * This is the simplest implementation I thought of.
         * A DC component will be a constant offset in the raw samples,
         * so correcting for it is as simple as getting the average value
         * (which in a normal situation should be close to 0), and substract it.
         */

        /** @brief It just removes a DC spike using
         * an exponential moving average with double-precision state,
         * respecting each stream type.
         * @param alpha 0.0001
         */
        template <typename T>
        class CorrectIQBlock : public BlockSimple<T, T>
        {
        private:
            double alpha = 0.0001;
            double acc_i = 0;
            double acc_q = 0;

        public:
            CorrectIQBlock();
            ~CorrectIQBlock();

            uint32_t process(T *input, uint32_t nsamples, T *output);

            void init()
            {
                acc_i = 0;
                acc_q = 0;
            }

            nlohmann::json get_cfg(std::string key) { throw satdump_exception(key); }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
