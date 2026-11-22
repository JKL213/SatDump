#include "transit_product_handler.h"
#include "logger.h"
#include "module_transit_demod.h"
#include "module_transit_decoder.h"
#include "core/config.h"
#include "imgui/implot/implot_internal.h"
#include "imgui/imgui_internal.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv)
{
    if (argc != 2) throw std::runtime_error("Usage: transit_product_test product.cbor");
    logger = std::make_shared<slog::Logger>();
    auto p = satdump::products::loadProduct(argv[1]);
    if (p->type != "transit_telemetry" || p->contents.at("subframes").size() != p->contents.at("summary").at("accepted_subframes").get<size_t>())
        throw std::runtime_error("Invalid TRANSIT product");
    ImGui::CreateContext();
    ImPlot::CreateContext();
    auto &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1200, 1600);
    io.DeltaTime = 1.0f / 60;
    io.IniFilename = nullptr;
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    {
        auto empty = std::make_shared<satdump::products::Product>();
        empty->type = p->type; empty->instrument_name = p->instrument_name; empty->contents = p->contents;
        empty->contents["subframes"] = nlohmann::json::array();
        empty->contents["commutator"] = nlohmann::json::array();
        transit::TransitProductHandler empty_handler(empty, false);
        transit::TransitProductHandler handler(p, false);
        // Reproduce opening an empty product first, then real data in the same
        // Explorer window. Plot IDs/ranges must not leak between the handlers.
        for (int pass = 0; pass < 3; ++pass)
        {
            ImGui::NewFrame();
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("TRANSIT test");
            auto &current = pass == 0 ? empty_handler : handler;
            current.drawMenu(); current.drawContents(io.DisplaySize);
            if (pass != 0)
            {
                ImGui::PushID(&handler);
                auto *plot = ImPlot::GetPlot("Frame counter (modulo 65536)");
                if (!plot) throw std::runtime_error("Counter plot not rendered");
                for (const auto &row : p->contents.at("subframes"))
                {
                    if (!row.contains("counter")) continue;
                    double t = row.at("symbol_time_s"), y = row.at("counter");
                    if (!plot->Axes[ImAxis_X1].Range.Contains(t) || !plot->Axes[ImAxis_Y1].Range.Contains(y))
                        throw std::runtime_error("Saved telemetry lies outside initial plot axes");
                }
                ImGui::PopID();
            }
            ImGui::End(); ImGui::Render();
            if (!ImGui::GetDrawData()->Valid) throw std::runtime_error("Product UI did not render");
        }
    }
    {
        satdump::satdump_cfg.main_cfg["user_interface"]["show_waterfall_demod_fft"]["value"] = false;
        transit::TransitDemodModule demod("", "", {{"samplerate", 2048000}, {"freq_shift", -527844}});
        demod.setInputType(satdump::pipeline::DATA_DSP_STREAM);
        demod.setOutputType(satdump::pipeline::DATA_FILE);
        const auto stats = demod.getModuleStats();
        if (stats.contains("snr") || stats.at("synced").get<bool>() || stats.at("subframes").get<int>() != 0)
            throw std::runtime_error("Uninitialized demodulator must not report signal quality or lock");
        transit::TransitDecoderModule decoder("", "", nlohmann::json::object());
        // Exercise every tab at wide, narrow and scaled sizes. Content must
        // remain reachable vertically without a horizontal status strip.
        auto check_tabs = [&](auto &module, const char *window_name, const char *bar_name, int tab_count, bool embedded)
        {
            for (float scale : {1.0f, 1.5f})
            {
                ui_scale = scale;
                io.FontGlobalScale = scale;
                for (float panel_width : {1450.0f, 480.0f, 320.0f})
                    for (int tab_index = 0; tab_index < tab_count; ++tab_index)
                        for (int pass = 0; pass < 4; ++pass)
                        {
                            auto *parent = ImGui::FindWindowByName(window_name);
                            if (parent)
                                if (auto *tabs = ImGui::TabBarFindByID(parent->GetID(bar_name)))
                                    if (auto *tab = ImGui::TabBarFindTabByOrder(tabs, tab_index))
                                        ImGui::TabBarQueueFocus(tabs, tab);
                            ImGui::NewFrame();
                            ImGui::SetNextWindowPos(ImVec2(0, 0));
                            ImGui::SetNextWindowSize(ImVec2(panel_width, 300));
                            module.drawUI(!embedded);
                            ImGui::Render();
                            if (!ImGui::GetDrawData()->Valid) throw std::runtime_error("Decoder UI did not render");
                            if (pass != 3) continue;
                            parent = ImGui::FindWindowByName(window_name);
                            auto *tabs = ImGui::TabBarFindByID(parent->GetID(bar_name));
                            if (!tabs || tabs->Tabs.Size != tab_count ||
                                tabs->SelectedTabId != ImGui::TabBarFindTabByOrder(tabs, tab_index)->ID)
                                throw std::runtime_error("Decoder tab missing or not selected");
                            if (parent->ScrollMax.x > 0 || parent->ScrollMax.y > 0)
                                throw std::runtime_error(std::string("Decoder footer or tabs outside parent panel: ") + window_name +
                                    " tab " + std::to_string(tab_index) + " width " + std::to_string(panel_width) +
                                    " scale " + std::to_string(scale) + " scroll " + std::to_string(parent->ScrollMax.x) +
                                    "," + std::to_string(parent->ScrollMax.y));
                            bool found = false;
                            for (auto *w : ImGui::GetCurrentContext()->Windows)
                            {
                                if (!w->Active || w->ParentWindow != parent) continue;
                                found = true;
                                if (w->ScrollMax.x > 0 || w->ContentSize.x > w->InnerRect.GetWidth() + 1)
                                    throw std::runtime_error(std::string("Horizontal overflow in ") + w->Name);
                                if (w->ContentSize.y > w->InnerRect.GetHeight() && w->ScrollMax.y <= 0)
                                    throw std::runtime_error("Tall tab content cannot scroll vertically");
                            }
                            if (!found) throw std::runtime_error("Decoder tab content missing");
                        }
            }
            ui_scale = 1;
            io.FontGlobalScale = 1;
        };
        check_tabs(demod, "TRANSIT 5E-5 PM/FSK (experimental)", "Tuning", 4, false);
        check_tabs(demod, "TRANSIT 5E-5 PM/FSK (experimental)", "Tuning", 4, true);
        demod.setInputType(satdump::pipeline::DATA_FILE);
        check_tabs(demod, "TRANSIT 5E-5 PM/FSK (experimental)", "Tuning", 4, true);
        check_tabs(decoder, "TRANSIT 5E-5 Telemetry (experimental)", "Telemetry tabs", 3, false);
        std::cout << "All decoder tabs fit 1450/480/320 px panels at 1x/1.5x scale\n";
    }
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    std::cout << "TRANSIT product load and headless UI render passed\n";
}
