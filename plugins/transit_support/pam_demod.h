#pragma once
#include "pam_decoder.h"
#include "common/dsp/filter/firdes.h"
#include <complex>

namespace transit
{
    // Streaming tap at the existing 64 ksample/s RF phase discriminator.
    // FIRs are evaluated only at their decimated output instants.
    class PamDemod
    {
        std::vector<float> channel_taps, audio_taps, audio;
        std::vector<std::complex<float>> channel;
        std::complex<float> oscillator{1, 0}, step, previous{0, 0};
        size_t cpos = 0, apos = 0;
        uint64_t count = 0, decimated = 0;
        double delay;
        float envelope_average = 0;
    public:
        std::function<void(PamSample)> on_sample;
        explicit PamDemod(double center = 5450)
        {
            channel_taps = dsp::firdes::low_pass(1, 64000, 900, 500);
            audio_taps = dsp::firdes::low_pass(1, 4000, 400, 150);
            channel.resize(channel_taps.size()); audio.resize(audio_taps.size());
            step = std::polar(1.0f, float(-2 * M_PI * center / 64000));
            delay = (channel_taps.size() - 1) / 128000.0 + .5 / 4000 + (audio_taps.size() - 1) / 8000.0;
        }
        void push(float x)
        {
            channel[cpos] = x * oscillator;
            cpos = (cpos + 1) % channel.size();
            oscillator *= step;
            if (++count % 16) return;
            if (count % 4096 == 0) oscillator /= std::abs(oscillator);
            std::complex<float> z{0, 0};
            size_t p = cpos;
            for (float tap : channel_taps) { p = p ? p - 1 : channel.size() - 1; z += channel[p] * tap; }
            float envelope = std::abs(z);
            envelope_average += .001f * (envelope - envelope_average);
            float hz = std::arg(z * std::conj(previous)) * float(4000 / (2 * M_PI));
            previous = z;
            audio[apos] = hz; apos = (apos + 1) % audio.size();
            if (++decimated % 4) return;
            float filtered = 0; p = apos;
            for (float tap : audio_taps) { p = p ? p - 1 : audio.size() - 1; filtered += audio[p] * tap; }
            double t = (count - 1) / 64000.0 - delay;
            if (t >= 0 && on_sample)
                on_sample({t, filtered, envelope, envelope > std::max(1e-5f, .12f * envelope_average) && std::abs(filtered) < 1000});
        }
    };
}
