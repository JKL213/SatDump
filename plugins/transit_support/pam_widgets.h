#pragma once
#include "pam_decoder.h"
#include "imgui/implot/implot.h"
#include "core/style.h"
#include <limits>

namespace transit
{
    inline const char *pam_label(int segment)
    {
        switch (segment)
        {
        case 2: return "Negative calibration level (observed)";
        case 30: return "Positive calibration level (observed)";
        case 5: case 6: return "Probable sun sensor pair; uncalibrated";
        case 7: return "Bus voltage hypothesis; not verified";
        case 8: return "Three-level status waveform; positions unassigned";
        case 33: case 34: return "Noisy in reference recording; interpretation unreliable";
        default: return "Analog channel; physical assignment unverified";
        }
    }
    inline void draw_pam_frame(const PamFrame &f)
    {
        std::array<double, 34> x, y;
        for (int k = 0; k < 34; ++k)
        { x[k] = k + 1; y[k] = f.coverage[k] >= .8 ? f.hz[k] : std::numeric_limits<double>::quiet_NaN(); }
        if (ImPlot::BeginPlot("PAM segments", ImVec2(-1, 150 * ui_scale)))
        {
            ImPlot::SetupAxes("Segment (sync excluded)", "Deviation (Hz)");
            ImPlot::SetupAxesLimits(.5, 34.5, -450, 500, ImPlotCond_Once);
            ImPlot::PlotBars("Median", x.data(), y.data(), 34, .7);
            if (ImPlot::IsPlotHovered())
            {
                int k = int(std::round(ImPlot::GetPlotMousePos().x)) - 1;
                if (k >= 0 && k < 34)
                    ImGui::SetTooltip("Segment %d: %s\n%.1f Hz | MAD %.1f Hz | usable %.0f%%", k + 1, pam_label(k + 1), f.hz[k], f.spread[k], 100 * f.coverage[k]);
            }
            ImPlot::EndPlot();
        }
        std::array<double, 66> t, status;
        for (int k = 0; k < 66; ++k)
        { t[k] = k * 5 + 2.5; status[k] = f.status_valid[k] ? f.status_hz[k] : std::numeric_limits<double>::quiet_NaN(); }
        if (ImPlot::BeginPlot("PAM segment 8: unassigned status levels", ImVec2(-1, 110 * ui_scale)))
        {
            ImPlot::SetupAxes("Time within segment (ms)", "Deviation (Hz)");
            ImPlot::SetupAxesLimits(0, 330, -450, 500, ImPlotCond_Once);
            ImPlot::PlotLine("5 ms medians", t.data(), status.data(), 66);
            ImPlot::EndPlot();
        }
    }
}
