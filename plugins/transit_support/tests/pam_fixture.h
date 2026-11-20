#pragma once
#include <cmath>
static float level(double t, bool calibration = true)
{
    if (t < .5) return 0;
    double phase = std::fmod(t - .5, 4864 / 390.594);
    if (phase < 512 / 390.594) return 350;
    double slot = (phase - 512 / 390.594) / (128 / 390.594);
    int k = int(slot);
    if (slot - k > .75) return 0;
    if (k == 1) return calibration ? -330 : 0;
    if (k == 29) return calibration ? 389 : 0;
    if (k == 7) return int((slot - k) * (128 / 390.594) / .02) % 3 == 0 ? -250 : 250;
    if (k == 0) return 7;
    return float(k * 5 - 60);
}
