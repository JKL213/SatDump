#include "correct_iq.h"
#include "core/exception.h"
#include <algorithm>
#include <cmath>

namespace dsp
{
    template <typename T>
    double CorrectIQBlock<T>::alpha_from_samplerate(double samplerate, double time_constant_seconds)
    {
        if (!std::isfinite(samplerate) || samplerate <= 0.0 || !std::isfinite(time_constant_seconds) || time_constant_seconds <= 0.0)
            throw satdump_exception("DC correction samplerate and time constant must be finite and positive.");
        double alpha = std::min(1.0, (1.0 / time_constant_seconds) / samplerate);
        if (!std::isfinite(alpha) || alpha <= 0.0)
            throw satdump_exception("DC correction alpha must be in (0, 1].");
        return alpha;
    }

    template <typename T>
    CorrectIQBlock<T>::CorrectIQBlock(std::shared_ptr<dsp::stream<T>> input, double alpha)
        : Block<T, T>(input), alpha(alpha)
    {
        if (!std::isfinite(alpha) || alpha <= 0.0 || alpha > 1.0)
            throw satdump_exception("DC correction alpha must be in (0, 1].");
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
            Block<T, T>::output_stream->writeBuf[i] = correct_iq_sample(Block<T, T>::input_stream->readBuf[i], alpha, acc_i, acc_q);

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(nsamples);
    }

    template class CorrectIQBlock<complex_t>;
    template class CorrectIQBlock<float>;
}
