#include "module_transit_decoder.h"
#include "core/exception.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "telemetry_widgets.h"
#include "products/product.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cmath>

namespace transit
{
    TransitDecoderModule::TransitDecoderModule(std::string i, std::string o, nlohmann::json p)
        : ProcessingModule(i, o, p), baud(p.value("baudrate", 390.594))
    {
        if (!std::isfinite(baud) || baud < 380 || baud > 400) throw satdump_exception("Invalid TRANSIT baudrate");
        channels.fill(-1);
    }

    void TransitDecoderModule::process()
    {
        std::ifstream in(d_input_file, std::ios::binary);
        if (!in) throw satdump_exception("Cannot open TRANSIT soft symbols");
        filesize = std::filesystem::file_size(d_input_file);
        std::string dir = d_output_file_hint + "_transit";
        std::filesystem::create_directories(dir);
        std::ofstream jsonl(dir + "/subframes.jsonl"), csv(dir + "/telemetry.csv"), comm(dir + "/commutator.csv"), events(dir + "/events.csv");
        if (!jsonl || !csv || !comm || !events) throw satdump_exception("Cannot create TRANSIT telemetry outputs");
        csv << "symbol_time_s,bit_offset,sf,encoder_format,counter,counter_step_ok,adc7_counts,adc15_counts,pam7_hz_est,pam15_hz_est,sun_gray_probable,flags12,flags13\n";
        comm << "symbol_time_s,word,channel_inferred,value12,encoder_format\n";
        events << "symbol_time_s,event,detail\n";
        satdump::products::Product product;
        product.type = "transit_telemetry";
        product.instrument_name = "transit_5e5";
        product.set_product_source("TRANSIT 5E-5");
        product.contents["subframes"] = nlohmann::json::array();
        product.contents["commutator"] = nlohmann::json::array();
        Deframer deframer;
        Commutator commutator;
        Frame prev;
        bool have_prev = false;
        uint64_t discontinuities = 0;
        auto accept = [&](Frame f)
        {
            double t = f.bit / baud;
            bool adjacent = have_prev && f.bit == prev.bit + 256 && f.sf != prev.sf;
            bool usable = f.state != 'X';
            bool counter_ok = adjacent && usable && prev.state != 'X' &&
                f.words[8] == uint16_t(prev.words[8] + (f.sf == 0 ? 1 : 0));
            if (!adjacent || !counter_ok || !usable) commutator.reset();
            if (have_prev && !adjacent)
            {
                ++discontinuities;
                events << t << ",sync_gap,unaccepted_bits=" << f.bit - prev.bit - 256 << '\n';
            }
            if (adjacent && usable && prev.state != 'X' && !counter_ok)
                events << t << ",counter_discontinuity,not_necessarily_a_reset\n";
            if (!have_prev || prev.state != f.state) events << t << ",encoder_format," << f.state << '\n';
            nlohmann::json j = {{"bit_offset", f.bit}, {"symbol_time_s", t}, {"subframe", f.sf}, {"inverted", f.inverted},
                {"encoder_format", std::string(1, f.state)}, {"raw_words", f.raw}, {"interpreted_words", f.words},
                {"counter_step_ok", adjacent && usable && prev.state != 'X' ? nlohmann::json(counter_ok) : nlohmann::json(nullptr)},
                {"crc_available", false}};
            csv << std::setprecision(10) << t << ',' << f.bit << ',' << f.sf << ',' << f.state << ',';
            if (usable)
            {
                auto a = adc_counts(f.words[6]), b = adc_counts(f.words[14]);
                double pa = (double(a) - 303) / 0.24, pb = (double(b) - 303) / 0.24;
                unsigned sun = sun_gray(f.words[12]);
                j["counter"] = f.words[8];
                j["counter_equivalent_seconds"] = f.words[8] * 512.0 / baud;
                j["pam_adc_counts"] = {a, b};
                j["pam_deviation_hz_estimate"] = {pa, pb};
                j["sun_gray_index_probable"] = sun;
                j["flags_w12"] = f.words[11]; j["flags_w13_low12"] = f.words[12] & 0xfff;
                csv << f.words[8] << ',';
                if (adjacent && prev.state != 'X') csv << counter_ok;
                csv << ',' << a << ',' << b << ',' << pa << ',' << pb << ',' << sun << ',' << f.words[11] << ',' << (f.words[12] & 0xfff);
                std::lock_guard<std::mutex> lock(ui_mutex);
                for (int w : {1, 5, 9, 13})
                {
                    unsigned value = f.words[w] >> 4;
                    int ch = commutator.push(value);
                    comm << std::setprecision(10) << (f.bit + w * 16) / baud << ',' << w + 1 << ',';
                    if (ch >= 0)
                    {
                        comm << ch; channels[ch] = value;
                        product.contents["commutator"].push_back({(f.bit + w * 16) / baud, ch, value});
                    }
                    comm << ',' << value << ',' << f.state << '\n';
                }
                auto append = [](std::vector<float> &v, float x)
                {
                    if (v.size() == 1024) v.erase(v.begin());
                    v.push_back(x);
                };
                append(counter_history, f.words[8]); append(sun_history, sun); append(pam_history, a);
            }
            else csv << ",,,,,,,,";
            csv << '\n';
            jsonl << j.dump() << '\n';
            product.contents["subframes"].push_back(j);
            {
                std::lock_guard<std::mutex> lock(ui_mutex);
                latest = f;
                if (f.state == 'H') ++healthy;
                else if (f.state == 'R') ++rotated;
                else ++suspect;
                ++frames;
            }
            prev = f; have_prev = true;
        };
        std::array<int8_t, 8192> buf;
        while (in.read(reinterpret_cast<char *>(buf.data()), buf.size()) || in.gcount())
        {
            auto n = in.gcount();
            for (std::streamsize i = 0; i < n; ++i) deframer.push(buf[i] > 0, accept);
            bytes += n;
        }
        if (in.bad()) throw satdump_exception("Error reading TRANSIT soft symbols");
        deframer.finish(accept);
        // PAM is an independent analog branch saved beside the input .soft.
        // A soft-only decode without this companion remains a PCM-only product.
        auto pam_path = std::filesystem::path(d_input_file).replace_extension(".pam.jsonl");
        product.contents["pam_frames"] = nlohmann::json::array();
        if (std::filesystem::exists(pam_path))
        {
            try
            {
                std::ifstream pam_in(pam_path);
                std::string line;
                bool metadata = false;
                while (std::getline(pam_in, line))
                {
                    auto r = nlohmann::json::parse(line);
                    if (r.at("kind") == "metadata")
                    {
                        if (metadata || r.at("version") != 1 || r.at("source") != "analog_subcarrier")
                            throw satdump_exception("Unsupported PAM companion metadata");
                        product.contents["pam_metadata"] = r; metadata = true;
                    }
                    else if (r.at("kind") == "frame")
                    {
                        if (!metadata || r.at("segments_hz").size() != 34 || r.at("mad_hz").size() != 34 ||
                            r.at("valid_fraction").size() != 34 || r.at("status8_hz_5ms").size() != 66)
                            throw satdump_exception("Invalid PAM frame shape");
                        double start = r.at("start_s").get<double>(), period = r.at("period_s").get<double>();
                        if (!r.at("following_sync").is_boolean()) throw satdump_exception("Invalid PAM sync evidence");
                        if (!std::isfinite(start) || start < 0 || !std::isfinite(period) || period < 11 || period > 14)
                            throw satdump_exception("Invalid PAM frame timing");
                        for (const char *key : {"segments_hz", "mad_hz", "valid_fraction", "status8_hz_5ms"})
                            for (const auto &v : r.at(key))
                            {
                                bool nullable = std::string(key) == "segments_hz" || std::string(key) == "status8_hz_5ms";
                                if (nullable && v.is_null()) continue;
                                if (!v.is_number() || !std::isfinite(v.get<double>()))
                                    throw satdump_exception("Invalid PAM sample value");
                            }
                        product.contents["pam_frames"].push_back(r);
                    }
                    else if (r.at("kind") == "end") product.contents["pam_summary"] = r;
                }
                if (pam_in.bad() || !metadata) throw satdump_exception("Cannot read PAM companion");
            }
            catch (const std::exception &e)
            {
                product.contents["pam_import_warning"] = e.what();
                logger->warn("TRANSIT PAM companion: %s", e.what());
            }
        }
        nlohmann::json summary = {
            {"satellite", "TRANSIT 5E-5 (experimental identification from supplied notes)"}, {"norad", 965},
            {"baudrate", baud}, {"input_symbols", bytes.load()}, {"accepted_subframes", frames.load()},
            {"analog_pam_frames", product.contents["pam_frames"].size()},
            {"analog_pam_present", product.contents.contains("pam_metadata")},
            {"format_H", healthy}, {"format_R", rotated}, {"format_X", suspect}, {"sync_gaps", discontinuities},
            {"unaccepted_symbols", bytes.load() - frames.load() * 256},
            {"source", "TRANSIT_5E5_Signalwissen.md, 2026-10-26; user-supplied measurements/interpretations"},
            {"limitations", {"No CRC: sync and ADC formats cannot prove error-free words.",
                "H/R/X describe ADC format evidence; X is suspect, not a proven extra clock.",
                "R words are rotated right once; raw words are retained. Counter continuity is checked separately.",
                "PCM times are relative to recovered symbols; DSP delay/acquisition make them approximate recording times.",
                "Counter-equivalent seconds are modulo 65536 frames, not proven uptime or a reset epoch.",
                "PAM ADC fields are PCM proxies (303 counts at zero, 0.24 counts/Hz, about 160 ms latency).",
                "Analog PAM, when present, is imported from the sibling .pam.jsonl; it has a separate sample time axis. No absolute engineering calibration.",
                "PAM segment 8 contains measured levels; no individual status positions or boom states are assigned.",
                "Channels use indices 0..41 as in the supplied CSV; marker = CH1. Two markers 42 values apart are required; discontinuities lose numbering.",
                "Sun Gray index is a probable sensor interpretation; no absolute attitude or spin fit."}},
            {"channel_hypotheses", {{"18/19", "probable main solar surface current; raw counts only"},
                {"38", "probable analog sun angle"}, {"2/32/35", "hypothesis: thermistors"},
                {"12/13 and 24/25", "hypothesis: temperature or regulated voltage"}}}
        };
        d_output_file = dir + "/summary.json";
        std::ofstream out(d_output_file);
        out << summary.dump(2) << '\n';
        out.close(); jsonl.close(); csv.close(); comm.close(); events.close();
        if (!out || !jsonl || !csv || !comm || !events) throw satdump_exception("Error writing TRANSIT telemetry outputs");
        product.contents["summary"] = summary;
        product.save(dir);
        d_output_file = dir + "/product.cbor";
        if (!std::filesystem::exists(d_output_file) || std::filesystem::file_size(d_output_file) == 0)
            throw satdump_exception("Cannot write TRANSIT product");
        logger->info("TRANSIT: %llu subframes, H=%llu R=%llu X=%llu, PAM=%llu", (unsigned long long)frames.load(),
            (unsigned long long)healthy, (unsigned long long)rotated, (unsigned long long)suspect,
            (unsigned long long)product.contents["pam_frames"].size());
    }
    nlohmann::json TransitDecoderModule::getModuleStats()
    {
        return {{"subframes", frames.load()}, {"progress", filesize ? double(bytes.load()) / filesize : 0.0}};
    }
    void TransitDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("TRANSIT 5E-5 Telemetry (experimental)", nullptr, window ? 0 : NOWINDOW_FLAGS);
        std::lock_guard<std::mutex> lock(ui_mutex);
        if (ImGui::BeginTabBar("Telemetry tabs", ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_TabListPopupButton))
        {
            if (ImGui::BeginTabItem("Overview"))
            {
                ImGui::BeginChild("Overview panel", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
                ImGui::Text("Subframes: %llu", (unsigned long long)frames.load());
                ImGui::Text("Normal (H): %llu", (unsigned long long)healthy);
                ImGui::Text("Rotated (R): %llu", (unsigned long long)rotated);
                ImGui::Text("Suspect (X): %llu", (unsigned long long)suspect);
                if (frames == 0) ImGui::TextDisabled("Waiting for telemetry...");
                frame_details(latest, frames != 0);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("History"))
            {
                ImGui::BeginChild("History panel", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
                if (counter_history.empty()) ImGui::TextWrapped("Waiting for accepted H/R samples...");
                else
                {
                    ImGui::TextWrapped("Accepted H/R samples; gaps omitted. ADC counts are uncalibrated.");
                    ImGui::TextUnformatted("Frame counter");
                    ImGui::PlotLines("##Counter", counter_history.data(), counter_history.size(), 0, nullptr, 0, 65535, ImVec2(-1, 80 * ui_scale));
                    ImGui::TextUnformatted("Sun Gray (probable)");
                    ImGui::PlotLines("##Sun", sun_history.data(), sun_history.size(), 0, nullptr, 0, 15, ImVec2(-1, 80 * ui_scale));
                    ImGui::TextUnformatted("PAM ADC w7");
                    ImGui::PlotLines("##ADC", pam_history.data(), pam_history.size(), 0, nullptr, 0, 1023, ImVec2(-1, 80 * ui_scale));
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Channels"))
            {
                ImGui::BeginChild("Channels panel", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
                ImGui::TextWrapped("Digital commutator: last observed counts (may be stale).");
                if (ImGui::BeginTable("Commutator", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
                {
                    ImGui::TableSetupColumn("Channel");
                    ImGui::TableSetupColumn("Counts");
                    ImGui::TableHeadersRow();
                    for (int i = 0; i < 42; ++i)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("CH%02d", i);
                        ImGui::TableNextColumn();
                        if (channels[i] >= 0) ImGui::Text("%d", channels[i]);
                        else ImGui::TextDisabled("--");
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::ProgressBar(filesize ? float(bytes.load()) / filesize : 0.0f);
        ImGui::End();
    }
}
