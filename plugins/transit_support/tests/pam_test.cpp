#include "pam_decoder.h"
#include "pam_demod.h"
#include <iostream>
#include <stdexcept>

using namespace transit;
static void require(bool v, const char *message) { if (!v) throw std::runtime_error(message); }
#include "pam_fixture.h"
int main()
{
    PamDecoder clean, bad, partial, dropout, reset;
    for (int i = 0; i < 39000; ++i)
    {
        double t = i / 1000.0;
        clean.push({t, level(t), .1f, true});
        bad.push({t, level(t, false), .1f, true});
        if (t < 13.5) partial.push({t, level(t), .1f, true});
        dropout.push({t, level(t), 0, false});
        reset.push({t, level(t + (t > 20 ? .2 : 0)), .1f, true});
    }
    clean.finish(); partial.finish(); bad.finish(); dropout.finish(); reset.finish();
    require(clean.frames == 3, "Three complete PAM frames expected");
    require(std::abs(clean.latest.hz[1] + 330) < 1 && std::abs(clean.latest.hz[29] - 389) < 1, "Calibration channels misnumbered");
    require(partial.frames == 0, "Truncated first frame must not acquire on one sync");
    require(bad.frames == 0, "Sync plateaus without calibration must not acquire");
    require(dropout.frames == 0, "Missing subcarrier must not produce frames");
    require(reset.discontinuities >= 1 && reset.frames < clean.frames, "Shortened reset interval must invalidate channel timing");
    // Exercise the actual mixer, channel FIR, FM discriminator and 400 Hz FIR.
    PamDecoder decoded;
    PamDemod dsp;
    dsp.on_sample = [&](PamSample s) { decoded.push(s); };
    double phase = 0;
    for (int i = 0; i < 39 * 64000; ++i)
    {
        double t = i / 64000.0;
        phase += 2 * M_PI * (5450 + level(t)) / 64000;
        dsp.push(.1 * std::cos(phase) + .2 * std::cos(2 * M_PI * 10600 * t));
    }
    decoded.finish();
    require(decoded.frames == 3, "PAM DSP failed with stronger PCM subcarrier present");
    require(std::abs(decoded.latest.hz[1] + 330) < 3 && std::abs(decoded.latest.hz[29] - 389) < 3, "PAM DSP frequency scale incorrect");
    std::cout << "PAM tests passed: sync, calibration, missing signal, truncated tail, reset, filtered subcarrier\n";
}
