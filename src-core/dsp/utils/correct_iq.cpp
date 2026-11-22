#include "correct_iq.h"
#include "common/dsp/complex.h"
#include "common/dsp/utils/correct_iq.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        CorrectIQBlock<T>::CorrectIQBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "correct_iq_cc" : "correct_iq_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        CorrectIQBlock<T>::~CorrectIQBlock()
        {
        }

        template <typename T>
        uint32_t CorrectIQBlock<T>::process(T *in, uint32_t nsamples, T *out)
        {
            for (uint32_t i = 0; i < nsamples; i++)
                out[i] = dsp::correct_iq_sample(in[i], alpha, acc_i, acc_q);
            return nsamples;
        }
        template class CorrectIQBlock<complex_t>;
        template class CorrectIQBlock<float>;
    } // namespace ndsp
} // namespace satdump
