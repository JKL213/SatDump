#pragma once
#include "handlers/product/product_handler.h"
#include "imgui/implot/implot.h"
#include <array>
#include "telemetry_widgets.h"
#include "pam_widgets.h"
#include <algorithm>

namespace transit
{
    // A native product handler keeps telemetry accessible after the pipeline ends.
    class TransitProductHandler : public satdump::handlers::ProductHandler
    {
        struct Series { std::vector<double> t, y; };
        std::array<Series, 6> fields;
        std::array<Series, 42> comm;
        std::array<Series, 34> pam_channels;
        int pam_frame = 0, pam_channel = 1;
        int channel = 0, selected_frame = 0;
        bool fit_requested = false;
        void do_process() override {}
        void plot(const char *title, const char *unit, const Series &s)
        {
            if (ImPlot::BeginPlot(title, ImVec2(-1, 180)))
            {
                ImPlot::SetupAxes("Recovered-symbol time (s)", unit);
                if (!s.t.empty())
                {
                    auto xr = std::minmax_element(s.t.begin(), s.t.end());
                    auto yr = std::minmax_element(s.y.begin(), s.y.end());
                    double xp = std::max(0.5, (*xr.second - *xr.first) * 0.025);
                    double yp = std::max(0.5, (*yr.second - *yr.first) * 0.05);
                    auto condition = fit_requested ? ImPlotCond_Always : ImPlotCond_Once;
                    ImPlot::SetupAxisLimits(ImAxis_X1, *xr.first - xp, *xr.second + xp, condition);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, *yr.first - yp, *yr.second + yp, condition);
                }
                // Points preserve sync gaps rather than implying interpolation.
                if (!s.t.empty())
                {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.5f);
                    ImPlot::PlotScatter(title, s.t.data(), s.y.data(), int(s.t.size()));
                }
                else ImPlot::PlotText("No samples", 0.5, 0.5);
                ImPlot::EndPlot();
            }
        }
    public:
        TransitProductHandler(std::shared_ptr<satdump::products::Product> p, bool dataset_mode)
            : ProductHandler(p, dataset_mode)
        {
            for (const auto &f : p->contents.at("subframes"))
            {
                double t = f.at("symbol_time_s");
                auto state = f.at("encoder_format").get<std::string>();
                fields[0].t.push_back(t); fields[0].y.push_back(state == "H" ? 0 : state == "R" ? 1 : 2);
                if (state == "X") continue;
                std::array<double, 5> values = {f.at("counter").get<double>(), f.at("sun_gray_index_probable").get<double>(),
                    f.at("pam_adc_counts").at(0).get<double>(), f.at("pam_adc_counts").at(1).get<double>(), f.at("flags_w12").get<double>()};
                for (size_t i = 0; i < values.size(); ++i) { fields[i + 1].t.push_back(t); fields[i + 1].y.push_back(values[i]); }
            }
            if (p->contents.contains("pam_frames"))
                for (const auto &r : p->contents.at("pam_frames"))
                    for (int k = 0; k < 34; ++k)
                        if (!r.at("segments_hz").at(k).is_null())
                        {
                            pam_channels[k].t.push_back(r.at("start_s"));
                            pam_channels[k].y.push_back(r.at("segments_hz").at(k));
                        }
            for (const auto &r : p->contents.at("commutator"))
            {
                int ch = r.at(1);
                if (ch < 0 || ch > 41) continue;
                comm[ch].t.push_back(r.at(0)); comm[ch].y.push_back(r.at(2));
            }
        }
        void drawMenu() override
        {
            if (ImGui::Button("Fit data")) fit_requested = true;
            ImGui::SliderInt("Commutator channel", &channel, 0, 41, "CH%d");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raw counts. CH18/19: probable solar current; CH38: probable sun angle.");
        }
        void drawContents(ImVec2) override
        {
            ImGui::PushID(this); // Never reuse axes from another (possibly empty) product.
            auto &s = product->contents.at("summary");
            ImGui::Text("Subframes %llu | H %llu | R %llu | X %llu", s.at("accepted_subframes").get<unsigned long long>(),
                s.at("format_H").get<unsigned long long>(), s.at("format_R").get<unsigned long long>(), s.at("format_X").get<unsigned long long>());
            auto &rows = product->contents.at("subframes");
            if (!rows.empty())
            {
                ImGui::SliderInt("Frame", &selected_frame, 0, int(rows.size()) - 1);
                auto &r = rows.at(selected_frame);
                Frame f;
                f.sf = r.at("subframe"); f.state = r.at("encoder_format").get<std::string>().at(0);
                f.raw = r.at("raw_words").get<std::array<uint16_t, 16>>();
                f.words = r.at("interpreted_words").get<std::array<uint16_t, 16>>();
                begin_status_row();
                ImGui::Text("t = %.3f s", r.at("symbol_time_s").get<double>());
                ImGui::SameLine(); format_lamps(f, true);
                ImGui::SameLine(); frame_values(f, true);
                ImGui::SameLine();
                status_flags(f, true);
                end_status_row();
            }
            if (ImGui::CollapsingHeader("Interpretation and limits"))
                for (const auto &v : s.at("limitations")) ImGui::TextWrapped("%s", v.get<std::string>().c_str());
            if (product->contents.contains("pam_import_warning"))
                ImGui::TextWrapped("PAM import: %s", product->contents.at("pam_import_warning").get<std::string>().c_str());
            if (product->contents.contains("pam_metadata") && product->contents.at("pam_frames").empty())
                ImGui::TextDisabled("PAM: no validated frames. The .pam_waveform.csv companion retains the analog measurements.");
            if (product->contents.contains("pam_frames") && !product->contents.at("pam_frames").empty() &&
                ImGui::CollapsingHeader("Analog PAM", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto &pr = product->contents.at("pam_frames");
                ImGui::SliderInt("PAM frame", &pam_frame, 0, int(pr.size()) - 1);
                auto &r = pr.at(pam_frame);
                PamFrame f; f.start = r.at("start_s"); f.period = r.at("period_s"); f.following_sync = r.at("following_sync");
                f.coverage = r.at("valid_fraction").get<std::array<float, 34>>();
                f.spread = r.at("mad_hz").get<std::array<float, 34>>();
                for (int k = 0; k < 34; ++k) if (!r.at("segments_hz").at(k).is_null()) f.hz[k] = r.at("segments_hz").at(k);
                for (int k = 0; k < 66; ++k)
                    if (!r.at("status8_hz_5ms").at(k).is_null()) { f.status_valid[k] = true; f.status_hz[k] = r.at("status8_hz_5ms").at(k); }
                ImGui::Text("Sample t %.3f s | %.3f s/frame | %s", f.start, f.period,
                    f.following_sync ? "following sync confirmed" : "complete tail, predicted end");
                draw_pam_frame(f);
                ImGui::SliderInt("PAM channel", &pam_channel, 1, 34);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pam_label(pam_channel));
                plot(("PAM segment " + std::to_string(pam_channel)).c_str(), "Deviation (Hz)", pam_channels[pam_channel - 1]);
            }
            plot("Encoder format", "0=H, 1=R, 2=X", fields[0]);
            plot("Frame counter (modulo 65536)", "Count", fields[1]);
            plot("Sun Gray index (probable)", "Index 0..15", fields[2]);
            plot("PAM proxy w7", "ADC counts", fields[3]);
            plot("PAM proxy w15", "ADC counts", fields[4]);
            plot("Status word w12 (interpretation open)", "Raw word", fields[5]);
            plot(("Digital commutator CH" + std::to_string(channel)).c_str(), "12-bit counts", comm[channel]);
            if (!rows.empty() && ImGui::CollapsingHeader("Inspect received words"))
            {
                ImGui::SliderInt("Subframe index", &selected_frame, 0, int(rows.size()) - 1);
                auto &f = rows.at(selected_frame);
                ImGui::Text("Time %.3f s, SF%d, format %s", f.at("symbol_time_s").get<double>(), f.at("subframe").get<int>(), f.at("encoder_format").get<std::string>().c_str());
                for (int w = 0; w < 16; ++w)
                    ImGui::Text("w%02d received %04X | interpreted %04X", w + 1, f.at("raw_words").at(w).get<unsigned>(), f.at("interpreted_words").at(w).get<unsigned>());
            }
            fit_requested = false;
            ImGui::PopID();
        }
        std::string getID() override { return "transit_telemetry_handler"; }
    };
}
