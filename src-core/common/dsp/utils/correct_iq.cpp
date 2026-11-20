#include "correct_iq.h"
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace dsp
{
    template <typename T>
    CorrectIQBlock<T>::CorrectIQBlock(std::shared_ptr<dsp::stream<T>> input, float alpha)
        : Block<T, T>(input), alpha(alpha)
    {
        if (!std::isfinite(alpha) || alpha <= 0.0f || alpha > 1.0f)
            throw std::invalid_argument("DC correction alpha must be in (0, 1].");
    }

    template <typename T>
    CorrectIQBlock<T>::~CorrectIQBlock()
    {
    }

    template <typename T>
    void CorrectIQBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            const auto sample = Block<T, T>::input_stream->readBuf[i];
            if constexpr (std::is_same_v<T, complex_t>)
            {
                acc_i += alpha * (sample.real - acc_i);
                acc_q += alpha * (sample.imag - acc_q);
                Block<T, T>::output_stream->writeBuf[i] = complex_t(sample.real - acc_i, sample.imag - acc_q);
            }
            else
            {
                acc_i += alpha * (sample - acc_i);
                Block<T, T>::output_stream->writeBuf[i] = sample - acc_i;
            }
        }

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(nsamples);
    }

    template class CorrectIQBlock<complex_t>;
    template class CorrectIQBlock<float>;
}
