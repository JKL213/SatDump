#include "transit_product_handler.h"
#include "logger.h"
#include "module_transit_demod.h"
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
        // Both the screenshot-sized panel and a narrow panel must keep one
        // row of controls, with horizontal scrolling instead of hidden flags.
        for (float panel_width : {1450.0f, 480.0f})
            for (int pass = 0; pass < 3; ++pass)
            {
                ImGui::NewFrame();
                ImGui::SetNextWindowSize(ImVec2(panel_width, 300));
                demod.drawUI(true);
                ImGui::Render();
                if (!ImGui::GetDrawData()->Valid) throw std::runtime_error("Live demodulator UI did not render");
                if (pass != 2) continue; // ImGui updates scroll ranges next frame.
                bool found = false;
                for (auto *w : ImGui::GetCurrentContext()->Windows)
                {
                    if (!w->Active || !w->ParentWindow || !std::strstr(w->Name, "Telemetry status")) continue;
                    found = true;
                    if (w->ContentSize.y > ImGui::GetFrameHeight() + 1 || w->ScrollMax.y > 0)
                        throw std::runtime_error("Status controls wrapped or were vertically clipped");
                    if (panel_width < 500 && w->ScrollMax.x <= 0)
                        throw std::runtime_error("Narrow status row cannot scroll to all flags");
                    std::cout << "Status row: panel " << panel_width << ", content " << w->ContentSize.x
                              << " x " << w->ContentSize.y << "\n";
                }
                if (!found) throw std::runtime_error("Status row not rendered");
            }
    }
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    std::cout << "TRANSIT product load and headless UI render passed\n";
}
