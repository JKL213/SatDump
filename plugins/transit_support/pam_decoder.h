#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

namespace transit
{
    struct PamSample { double t; float hz, envelope; bool valid; };
    struct PamFrame
    {
        double start = 0, period = 0;
        bool following_sync = false;
        std::array<float, 34> hz{}, spread{}, coverage{};
        // Preserve the measured waveform; these are NOT assigned spacecraft bits.
        std::array<float, 66> status_hz{};
        std::array<bool, 66> status_valid{};
    };
    inline float pam_median(std::vector<float> &v)
    {
        if (v.empty()) return 0;
        auto m = v.begin() + v.size() / 2;
        std::nth_element(v.begin(), m, v.end());
        return *m;
    }
    class PamDecoder
    {
        double baud, run_start = -1, last_high = -1, anchor = -1, emitted = -1;
        bool tracking = false;
        std::deque<PamSample> history;
        std::vector<float> group;
        int group_valid = 0;
        void emit(double end, bool confirmed)
        {
            if (anchor < 0 || anchor == emitted) return;
            PamFrame f; f.start = anchor; f.period = end - anchor; f.following_sync = confirmed;
            for (int k = 0; k < 34; ++k)
            {
                double a = anchor + (512 + k * 128) / baud + 0.04;
                double b = anchor + (512 + k * 128) / baud + 0.21;
                std::vector<float> v;
                int total = 0;
                for (const auto &s : history) if (s.t >= a && s.t < b)
                { ++total; if (s.valid) v.push_back(s.hz); }
                f.coverage[k] = total ? float(v.size()) / total : 0;
                f.hz[k] = pam_median(v);
                for (auto &x : v) x = std::abs(x - f.hz[k]);
                f.spread[k] = pam_median(v); // median absolute deviation, Hz
            }
            double status_start = anchor + (512 + 7 * 128) / baud;
            for (int k = 0; k < 66; ++k)
            {
                std::vector<float> v;
                for (const auto &s : history)
                    if (s.valid && s.t >= status_start + k * .005 && s.t < status_start + (k + 1) * .005) v.push_back(s.hz);
                f.status_valid[k] = v.size() >= 3;
                f.status_hz[k] = pam_median(v);
            }
            // A plateau alone is not enough: require the observed negative and
            // positive calibration slots and a zero slot in their expected places.
            if (f.coverage[1] >= .8 && f.coverage[29] >= .8 && f.coverage[0] >= .8 &&
                f.hz[1] > -450 && f.hz[1] < -180 && f.hz[29] > 220 && f.hz[29] < 500 &&
                std::abs(f.hz[0]) < 100 && f.spread[1] < 40 && f.spread[29] < 40)
            {
                latest = f; ++frames; emitted = anchor;
                if (on_frame) on_frame(f);
            }
            else { ++rejected; tracking = false; }
        }
        void sync(double start)
        {
            if (anchor >= 0)
            {
                double dt = start - anchor;
                if (std::abs(dt - period()) <= .08)
                {
                    tracking = true;
                    emit(start, true);
                }
                else { tracking = false; ++discontinuities; }
            }
            anchor = start;
        }
    public:
        uint64_t frames = 0, rejected = 0, discontinuities = 0;
        double time = 0;
        PamFrame latest;
        std::function<void(const PamFrame &)> on_frame;
        explicit PamDecoder(double rate = 390.594) : baud(rate) {}
        double period() const { return 4864 / baud; }
        bool synced() const { return tracking && time - anchor < period() + 1.5; }
        const std::deque<PamSample> &samples() const { return history; }
        void push(PamSample s)
        {
            time = s.t;
            history.push_back(s);
            while (!history.empty() && history.front().t < time - 2 * period() - 2) history.pop_front();
            group.push_back(s.hz); group_valid += s.valid;
            if (group.size() < 10) return; // 10 ms robust sync detector at 1 ksample/s.
            float v = pam_median(group);
            bool high = group_valid >= 8 && v > 270 && v < 430;
            group.clear(); group_valid = 0;
            if (high)
            {
                if (run_start < 0) run_start = s.t - .009;
                last_high = s.t;
            }
            else if (run_start >= 0 && s.t - last_high >= .025)
            {
                double duration = last_high - run_start + .001;
                if (std::abs(duration - 512 / baud) < .12) sync(run_start);
                run_start = -1;
            }
        }
        void finish()
        {
            // Only a fully collected tail following an acquired sync is allowed.
            // No padding of truncated recordings; mark the missing next-sync check.
            if (synced() && time >= anchor + period()) emit(anchor + period(), false);
        }
    };
}
