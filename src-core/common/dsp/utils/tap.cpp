#include "tap.h"
#include <cstring>

namespace dsp
{
    template <typename T>
    TapBlock<T>::TapBlock(std::shared_ptr<dsp::stream<T>> input, std::function<void(T *, int)> callback)
        : Block<T, T>(input), callback(callback)
    {
    }

    template <typename T>
    TapBlock<T>::~TapBlock()
    {
    }

    template <typename T>
    void TapBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        if (callback)
            callback(Block<T, T>::input_stream->readBuf, nsamples);

        memcpy(Block<T, T>::output_stream->writeBuf, Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(nsamples);
    }

    template class TapBlock<complex_t>;
    template class TapBlock<float>;
} // namespace dsp
