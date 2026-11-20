#include "module_transit_demod.h"
#include "module_transit_decoder.h"
#include "core/config.h"
#include "logger.h"
#include "products/product.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/implot/implot.h"
#include "pam_fixture.h"

int main()
{
    logger = std::make_shared<slog::Logger>();
    satdump::satdump_cfg.main_cfg["user_interface"]["show_waterfall_demod_fft"]["value"] = false;
    const auto dir = std::filesystem::temp_directory_path() / ("transit-live-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    const auto prefix = (dir / "signal").string();
    transit::TransitDemodModule demod("", prefix, {{"samplerate", 64000}});
    demod.setInputType(satdump::pipeline::DATA_DSP_STREAM);
    demod.setOutputType(satdump::pipeline::DATA_FILE);
    demod.input_stream = std::make_shared<dsp::stream<complex_t>>();
    demod.init(); demod.input_active = true;
    std::thread worker([&] { demod.process(); });
    std::vector<bool> bits;
    for (int n = 0; n < 60; ++n)
    {
        std::array<uint16_t, 16> w{};
        w[0] = n % 2 ? 0xe25f : 0xe240; w[6] = 0x52f1; w[14] = 0x5591; w[8] = n / 2; w[12] = 0xb400;
        for (auto v : w) for (int b = 15; b >= 0; --b) bits.push_back((v >> b) & 1);
    }
    double phase = 0, pam_phase = 0;
    const int samples = int(bits.size() * 64000 / 390.594);
    for (int pos = 0; pos < samples;)
    {
        int n = std::min(4096, samples - pos);
        for (int i = 0; i < n; ++i)
        {
            double t = (pos + i) / 64000.0;
            bool b = bits.at(size_t(t * 390.594));
            phase = std::remainder(phase + 2 * M_PI * (b ? 11250 : 9950) / 64000, 2 * M_PI);
            pam_phase = std::remainder(pam_phase + 2 * M_PI * (5450 + level(t)) / 64000, 2 * M_PI);
            double angle = 2 * M_PI * 1000 * t + 0.4 * std::sin(phase) + 0.25 * std::sin(pam_phase);
            demod.input_stream->writeBuf[i] = complex_t(std::cos(angle), std::sin(angle));
        }
        demod.input_stream->swap(n); pos += n;
    }
    // Exercise the native FFT and lamps with real DSP output, not a null monitor.
    ImGui::CreateContext(); ImPlot::CreateContext();
    auto &io = ImGui::GetIO(); io.DisplaySize = ImVec2(1100, 700); io.DeltaTime = 1.0f / 60; io.IniFilename = nullptr;
    unsigned char *pixels; int width, height; io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ImGui::NewFrame(); ImGui::SetNextWindowSize(io.DisplaySize); demod.drawUI(true); ImGui::Render();
    bool rendered = ImGui::GetDrawData()->Valid;
    auto *window = ImGui::FindWindowByName("TRANSIT 5E-5 PM/FSK (experimental)");
    auto *tabs = ImGui::TabBarFindByID(window->GetID("Tuning"));
    if (!tabs) throw std::runtime_error("Tuning tabs missing");
    auto *pam_tab = ImGui::TabBarFindTabByOrder(tabs, 2);
    if (!pam_tab) throw std::runtime_error("PAM tab missing");
    ImGui::TabBarQueueFocus(tabs, pam_tab);
    ImGui::NewFrame(); demod.drawUI(true); ImGui::Render();
    rendered &= ImGui::GetDrawData()->Valid;
    ImPlot::DestroyContext(); ImGui::DestroyContext();
    // A flush must expose data while processing is still active, not just at close.
    auto before_stop = demod.getModuleStats();
    if (before_stop.at("pam_frames").get<int>() < 2 || before_stop.at("pam_write_error").get<bool>())
        throw std::runtime_error("Live PAM must acquire independently and save before stopping");
    {
        std::ifstream saved_pam(prefix + ".pam.jsonl");
        std::string line; int saved_frames = 0;
        while (std::getline(saved_pam, line)) if (nlohmann::json::parse(line).at("kind") == "frame") ++saved_frames;
        if (saved_frames < 2) throw std::runtime_error("PAM frames were not flushed while active");
    }
    auto size_before_stop = std::filesystem::file_size(prefix + ".soft");
    demod.input_active = false;
    demod.input_stream->stopReader(); demod.input_stream->stopWriter(); demod.stop(); worker.join();
    if (!rendered || !before_stop.at("carrier_candidate").get<bool>() ||
        std::abs(before_stop.at("carrier_offset_hz").get<double>() - 1000) > 100)
        throw std::runtime_error("Native tuning FFT must display the injected 1 kHz carrier offset");
    if (!before_stop.at("saved").get<bool>() || before_stop.at("write_error").get<bool>() ||
        before_stop.at("subframes").get<unsigned>() < 10 || size_before_stop == 0)
        throw std::runtime_error("Live frames must have on-disk symbols before stopping");
    transit::TransitDecoderModule decoder(prefix + ".soft", (dir / "decoded").string(), nlohmann::json::object());
    decoder.process();
    auto product = satdump::products::loadProduct(decoder.getOutput());
    auto count = product->contents.at("subframes").size();
    if (count != demod.getModuleStats().at("subframes").get<size_t>())
        throw std::runtime_error("Saved product lost live-confirmed frames");
    if (product->contents.at("pam_frames").size() != demod.getModuleStats().at("pam_frames").get<size_t>() ||
        product->contents.at("pam_frames").size() != 3)
        throw std::runtime_error("Saved product lost PAM frames or padded incomplete tail");
    std::cout << "Live PAM persistence passed: 3 analog frames\n";
    std::cout << "Live persistence passed: " << size_before_stop << " bytes before stop, " << count << " saved subframes\n";
    // A malformed optional companion must report an import warning without
    // breaking PCM or discarding previously validated analog frames.
    {
        std::ofstream corrupt(prefix + ".pam.jsonl", std::ios::app);
        corrupt << "{broken record\n";
    }
    transit::TransitDecoderModule damaged(prefix + ".soft", (dir / "damaged").string(), nlohmann::json::object());
    damaged.process();
    auto recovered = satdump::products::loadProduct(damaged.getOutput());
    if (!recovered->contents.contains("pam_import_warning") || recovered->contents.at("subframes").size() != count ||
        recovered->contents.at("pam_frames").size() != 3)
        throw std::runtime_error("Malformed optional PAM companion damaged the PCM product");
    std::filesystem::remove_all(dir);
}
