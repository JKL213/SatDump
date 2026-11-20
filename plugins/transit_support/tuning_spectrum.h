#pragma once
#include "common/dsp/path/splitter.h"
#include "common/dsp/fft/fft_pan.h"
#include "common/widgets/fft_plot.h"
#include <algorithm>
#include <mutex>

namespace transit
{
    // SatDump's FFT processing and native FFTPlot, tapped after channel filtering.
    // Snapshot the FFT in the callback so rendering never races the DSP writer.
    class TuningSpectrum
    {
        std::shared_ptr<dsp::SplitterBlock> splitter;
        std::shared_ptr<dsp::FFTPanBlock> fft;
        std::mutex mutex;
        std::vector<float> values, render_values;
        satdump::widgets::FFTPlot plot;
        float rate;
        bool ready = false;
    public:
        struct Peak { bool ready = false, visible = false; float hz = 0, prominence = 0; };
        TuningSpectrum(std::shared_ptr<dsp::stream<complex_t>> input, int samplerate, int size)
            : values(size, -100), render_values(size, -100), plot(render_values.data(), size, -100, 0, 10), rate(samplerate)
        {
            splitter = std::make_shared<dsp::SplitterBlock>(input);
            splitter->add_output("spectrum"); splitter->set_enabled("spectrum", true);
            fft = std::make_shared<dsp::FFTPanBlock>(splitter->get_output("spectrum"));
            fft->set_fft_settings(size, samplerate, samplerate == 4000 ? 8 : 20);
            fft->avg_num = 3;
            fft->on_fft = [this, size](float *v)
            {
                std::lock_guard<std::mutex> lock(mutex);
                std::copy(v, v + size, values.begin());
                ready = true;
            };
            plot.enable_freq_scale = false;
        }
        std::shared_ptr<dsp::stream<complex_t>> output() { return splitter->output_stream; }
        void start() { fft->start(); splitter->start(); }
        void stop()
        {
            splitter->get_output("spectrum")->stopWriter();
            fft->stop(); splitter->stop();
        }
        Peak peak()
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!ready) return {};
            // On RF, search the central +/- 12 kHz for a carrier candidate.
            int lo = rate > 4000 ? int(values.size() * (0.5f - 12000 / rate)) : 0;
            int hi = rate > 4000 ? int(values.size() * (0.5f + 12000 / rate)) : int(values.size());
            auto it = std::max_element(values.begin() + lo, values.begin() + hi);
            std::vector<float> floor(values.begin() + lo, values.begin() + hi);
            std::nth_element(floor.begin(), floor.begin() + floor.size() / 2, floor.end());
            float prominence = *it - floor[floor.size() / 2];
            return {true, prominence > 10, ((it - values.begin()) / float(values.size()) - 0.5f) * rate, prominence};
        }
        void draw(float height, const std::vector<std::pair<const char *, float>> &markers)
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                std::copy(values.begin(), values.end(), render_values.begin());
            }
            float top = *std::max_element(render_values.begin(), render_values.end());
            plot.scale_max = top + 5; plot.scale_min = top - 65;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 size(ImGui::GetContentRegionAvail().x, height);
            plot.draw(size);
            auto *dl = ImGui::GetWindowDrawList();
            for (auto marker : markers)
            {
                float x = pos.x + size.x * (0.5f + marker.second / rate);
                dl->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y), IM_COL32(235, 185, 55, 180));
                dl->AddText(ImVec2(x + 3, pos.y + 3), IM_COL32(245, 210, 110, 255), marker.first);
            }
            ImGui::Text("-%g kHz", rate / 2000);
            ImGui::SameLine(size.x * 0.48f); ImGui::TextUnformatted("0");
            ImGui::SameLine(size.x * 0.88f); ImGui::Text("+%g kHz", rate / 2000);
        }
    };
}
