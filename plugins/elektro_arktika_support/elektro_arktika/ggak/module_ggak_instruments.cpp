#include "module_ggak_instruments.h"
#include "ggak.h"
#include "imgui/imgui.h"
#include "imgui/implot/implot.h"
#include "skl.h"
#include "utils/time.h"
#include <cstdint>
#include <cstdio>

namespace elektro_arktika
{
    namespace ggak
    {
        GGAKInstrumentsDecoderModule::GGAKInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = true;
            // Keep file processing from overwriting the preceding decoder's CADU file.
            fsfsm_file_ext = ".ggak.cadu";

            is_arktika = parameters.contains("is_arktika") ? parameters["is_arktika"].get<bool>() : false;
            if (parameters.contains("satellite_number"))
                sat_num = parameters["satellite_number"].is_string() ? std::stoi(parameters["satellite_number"].get<std::string>()) : parameters["satellite_number"].get<int>();
            else
                sat_num = 0;
        }

        void GGAKInstrumentsDecoderModule::process()
        {
            GGAKFrame frm{};

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&frm, sizeof(frm));
                if (!should_run()) // EOF, incomplete frame or stopped input
                    break;

                {
                    std::lock_guard<std::mutex> lock(records_mtx);
                    last_frame = frm;
                    frames++;

                    if (!checkCRC(&frm))
                        crc_errors++;
                    else if (frm.id == 48)
                    {
                        auto recs = parseSKLRecord(&frm);
                        skl_records.insert(skl_records.end(), recs.begin(), recs.end());
                        // Only retain a bounded history for the live display.
                        if (skl_records.size() > 1024)
                            skl_records.erase(skl_records.begin(), skl_records.end() - 1024);
                    }
                }

                // Write to save cadu
                write_data((uint8_t *)&frm, sizeof(frm));
            }

            cleanup();
        }

        void GGAKInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("ELEKTRO / ARKTIKA GGAK Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            GGAKFrame frm;
            uint64_t frame_count, error_count;
            std::vector<SKLRecord> records;
            {
                std::lock_guard<std::mutex> lock(records_mtx);
                frm = last_frame;
                frame_count = frames;
                error_count = crc_errors;
                records = skl_records;
            }

            ImGui::Text("Frames: %llu / CRC errors: %llu", (unsigned long long)frame_count, (unsigned long long)error_count);
            if (frame_count == 0)
                ImGui::TextUnformatted("Waiting for GGAK frames...");
            else
            {
                ImGui::Text("ID: %u / Master: %u / Channel: %u", unsigned(frm.id), unsigned(frm.master_counter), unsigned(frm.channel_counter));
                ImGui::Text("Frame time: %s UTC / CRC: %s", satdump::timestamp_to_string(getTimestamp(&frm)).c_str(), checkCRC(&frm) ? "OK" : "ERROR");
            }

            if (ImGui::BeginTabBar("##ggaktabs"))
            {
                if (ImGui::BeginTabItem("Hex"))
                {
                    if (frame_count > 0)
                    {
                        const uint8_t *data = (const uint8_t *)&frm;
                        for (size_t i = 0; i < sizeof(frm); i += 16)
                        {
                            char line[64];
                            int pos = snprintf(line, sizeof(line), "%04X  ", unsigned(i));
                            for (size_t j = i; j < i + 16 && j < sizeof(frm); j++)
                                pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", unsigned(data[j]));
                            ImGui::TextUnformatted(line);
                        }
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("SKL"))
                {
                    if (records.empty())
                        ImGui::TextUnformatted("Waiting for valid SKL frames (ID 48)...");
                    else
                    {
                        ImGui::Text("Latest SKL frame: %s UTC", satdump::timestamp_to_string(records.back().time).c_str());
                        if (ImGui::BeginTable("##sklchannels", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                        {
                            ImGui::TableSetupColumn("Channel");
                            ImGui::TableSetupColumn("Latest value");
                            ImGui::TableHeadersRow();
                            for (int c = 0; c < 12; c++)
                            {
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                std::string label = "SKL " + std::to_string(c + 1);
                                if (c == 10) // The existing parser has no data for this channel.
                                    ImGui::TextUnformatted(label.c_str());
                                else if (ImGui::Selectable(label.c_str(), selected_channel == c))
                                    selected_channel = c;
                                ImGui::TableNextColumn();
                                if (c == 10)
                                    ImGui::TextDisabled("Unavailable");
                                else
                                    ImGui::Text("%.3f", records.back().dat[c]);
                            }
                            ImGui::EndTable();
                        }

                        if (ImPlot::BeginPlot("SKL history", ImVec2(-1, 200 * ui_scale)))
                        {
                            // The parser assigns one frame timestamp to all 14 samples.
                            ImPlot::SetupAxes("Sample (latest 1024)", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                            std::string label = "SKL " + std::to_string(selected_channel + 1);
                            ImPlot::PlotLine(label.c_str(), &records[0].dat[selected_channel], records.size(), 1, 0, 0, 0, sizeof(SKLRecord));
                            ImPlot::EndPlot();
                        }
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string GGAKInstrumentsDecoderModule::getID() { return "elektro_arktika_ggak_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GGAKInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GGAKInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ggak
} // namespace elektro_arktika
