#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>

namespace transit
{
    inline uint16_t ror1(uint16_t v) { return uint16_t((v >> 1) | (v << 15)); }
    inline bool adc_valid(uint16_t v) { return (v & 0xc00f) == 0x4001; }
    inline unsigned adc_counts(uint16_t v) { return (v >> 4) & 1023; }
    inline unsigned sun_gray(uint16_t word)
    {
        unsigned n = word >> 12, r = 0;
        for (int i = 0; i < 4; ++i) r = (r << 1) | ((n >> i) & 1);
        r ^= r >> 1;
        r ^= r >> 2;
        return r;
    }

    struct Frame
    {
        uint64_t bit = 0;
        int sf = 0;
        bool inverted = false;
        std::array<uint16_t, 16> raw{}, words{};
        char state = 'X'; // Format evidence, not an assertion of error-free telemetry.
    };

    inline void interpret(Frame &f)
    {
        f.words = f.raw;
        if (adc_valid(f.raw[6]) && adc_valid(f.raw[14])) f.state = 'H';
        else if (adc_valid(ror1(f.raw[6])) && adc_valid(ror1(f.raw[14])))
        {
            f.state = 'R';
            // Sync is generated separately; preserve the received word.
            for (int i = 1; i < 16; ++i) f.words[i] = ror1(f.raw[i]);
        }
        else f.state = 'X';
    }

    // Exact sync, polarity search, and a second alternating sync 256 bits later.
    // No fuzzy acquisition: there is no CRC to reject false candidates.
    class Deframer
    {
        std::deque<uint8_t> bits;
        uint64_t offset = 0;
        bool locked = false;
        uint16_t word(size_t pos, bool inv = false) const
        {
            uint16_t v = 0;
            for (size_t i = 0; i < 16; ++i) v = uint16_t((v << 1) | (bits[pos + i] ^ inv));
            return v;
        }
        static int sync(uint16_t w) { return w == 0xe240 ? 0 : w == 0xe25f ? 1 : -1; }
        void discard(size_t n) { while (n--) { bits.pop_front(); ++offset; } }
        void emit(bool inv, int sf, const std::function<void(Frame)> &out)
        {
            Frame f;
            f.bit = offset; f.sf = sf; f.inverted = inv;
            for (int i = 0; i < 16; ++i) f.raw[i] = word(i * 16, inv);
            interpret(f);
            out(f);
            discard(256);
        }
    public:
        void push(bool bit, const std::function<void(Frame)> &out)
        {
            bits.push_back(bit);
            while (bits.size() >= 272)
            {
                bool inv = sync(word(0)) < 0;
                int sf = sync(word(0, inv));
                if (sf >= 0 && sync(word(256, inv)) == 1 - sf)
                {
                    emit(inv, sf, out);
                    locked = true;
                }
                else { discard(1); locked = false; }
            }
        }
        void finish(const std::function<void(Frame)> &out)
        {
            // The final complete subframe has already been confirmed as the next
            // sync of the preceding frame. Never pad a truncated final frame.
            if (locked && bits.size() >= 256)
            {
                bool inv = sync(word(0)) < 0;
                int sf = sync(word(0, inv));
                if (sf >= 0) emit(inv, sf, out);
            }
        }
    };

    // Lightweight preview for the demodulator UI. It uses the same exact-sync
    // rules as the product decoder; symbol amplitudes alone do not imply lock.
    class LivePreview
    {
        Deframer deframer;
        void accept(Frame f)
        {
            latest = f;
            ++subframes;
            last_frame_end = f.bit + 256;
        }
    public:
        uint64_t symbols = 0, subframes = 0, last_frame_end = 0;
        Frame latest;
        std::array<float, 512> waveform{};
        size_t waveform_size = 0, waveform_next = 0;
        void push(int8_t soft)
        {
            ++symbols;
            waveform[waveform_next] = float(soft) / 90.0f;
            waveform_next = (waveform_next + 1) % waveform.size();
            if (waveform_size < waveform.size()) ++waveform_size;
            deframer.push(soft > 0, [this](Frame f) { accept(f); });
        }
        void finish() { deframer.finish([this](Frame f) { accept(f); }); }
        bool synced() const { return subframes != 0 && symbols <= last_frame_end + 512; }
    };

    // Number channels only after a marker repeats at the 42-value period.
    // Reset on any frame/counter discontinuity. Raw values remain available.
    class Commutator
    {
        int64_t index = 0, marker = -1;
        bool locked = false;
    public:
        void reset() { index = 0; marker = -1; locked = false; }
        int push(unsigned value)
        {
            bool is_marker = value >= 1642 && value <= 1652;
            if (locked && (index - marker) % 42 == 0 && !is_marker) locked = false;
            if (is_marker)
            {
                if (marker >= 0 && index - marker == 42) locked = true;
                // Ignore incidental marker-like samples away from the locked phase.
                if (!locked || (index - marker) % 42 == 0) marker = index;
            }
            int channel = locked ? int((index - marker + 1) % 42) : -1;
            ++index;
            return channel;
        }
    };
}
