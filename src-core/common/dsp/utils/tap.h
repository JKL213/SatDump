#pragma once

#include "common/dsp/block.h"
#include <functional>

namespace dsp
{
    /*
    Pass-through block that hands every buffer to a callback
    before forwarding it unchanged. Lets a second consumer
    (audio monitoring, etc.) look at a stream without breaking
    the single-reader rule.
    */
    template <typename T>
    class TapBlock : public Block<T, T>
    {
    private:
        void work();

    public:
        TapBlock(std::shared_ptr<dsp::stream<T>> input, std::function<void(T *, int)> callback = nullptr);
        ~TapBlock();

        std::function<void(T *, int)> callback;
    };
} // namespace dsp
