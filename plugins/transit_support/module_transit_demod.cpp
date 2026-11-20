#include "module_transit_demod.h"
#include "common/dsp/filter/firdes.h"
#include <cmath>
#include "telemetry_widgets.h"
#include "pam_widgets.h"

namespace transit
{
    SubcarrierMixer::SubcarrierMixer(std::shared_ptr<dsp::stream<float>> input, double rate, double center)
        : Block(input), step(-2 * M_PI * center / rate) {}
    void SubcarrierMixer::work()
    {
        int n = input_stream->read();
        if (n <= 0) { input_stream->flush(); return; }
        for (int i = 0; i < n; ++i)
        {
            float v = input_stream->readBuf[i];
            output_stream->writeBuf[i] = complex_t(v * cos(phase), v * sin(phase));
            phase = std::remainder(phase + step, 2 * M_PI);
        }
        input_stream->flush();
        output_stream->swap(n);
    }

    TransitDemodModule::TransitDemodModule(std::string i, std::string o, nlohmann::json p)
        : BaseDemodModule(i, o, p), baud(p.value("baudrate", 390.594)),
          center(p.value("pcm_center", 10600.0)), deviation(p.value("pcm_deviation", 650.0)), pam_center(p.value("pam_center", 5450.0))
    {
        if (!std::isfinite(baud) || baud < 380 || baud > 400 || !std::isfinite(center) || center < 9000 || center > 12000 ||
            !std::isfinite(deviation) || deviation < 100 || deviation > 1000 || !std::isfinite(pam_center) || pam_center < 5000 || pam_center > 5900)
            throw satdump_exception("Invalid TRANSIT baudrate or PCM frequencies");
        if (d_samplerate < 30000) throw satdump_exception("TRANSIT requires at least 30 ksample/s IQ; 64k or 250k recommended");
        name = "TRANSIT 5E-5 PM/FSK (experimental)";
        // BaseDemod's symbolrate is integer. Preserve the fractional PCM baud
        // separately and keep the entire RF signal until after the FM discriminator.
        d_symbolrate = 390;
        MIN_SPS = MAX_SPS = 1;
        d_parameters["min_sps"] = 1;
        d_parameters["max_sps"] = 1;
        d_parameters["custom_samplerate"] = 64000;
    }
    void TransitDemodModule::init()
    {
        BaseDemodModule::initb();
        fine_tune = std::make_shared<dsp::FreqShiftBlock>(agc->output_stream, final_samplerate, 0);
        rf_spectrum = std::make_unique<TuningSpectrum>(fine_tune->output_stream, final_samplerate, 2048);
        // Differentiating PM preserves the two subcarrier frequencies. RF Doppler
        // becomes a slow/DC term rejected by the subcarrier channel filter.
        rf_demod = std::make_shared<dsp::QuadratureDemodBlock>(rf_spectrum->output(), 1.0f);
        pam = std::make_shared<PamTap>(rf_demod->output_stream, pam_center, baud, d_output_file_hint);
        mixer = std::make_shared<SubcarrierMixer>(pam->output_stream, final_samplerate, center);
        pcm_resampler = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(mixer->output_stream, 4000, final_samplerate);
        pcm_spectrum = std::make_unique<TuningSpectrum>(pcm_resampler->output_stream, 4000, 512);
        fsk_demod = std::make_shared<dsp::QuadratureDemodBlock>(pcm_spectrum->output(), 4000 / (2 * M_PI * deviation));
        filter = std::make_shared<dsp::FIRBlock<float>>(fsk_demod->output_stream, dsp::firdes::low_pass(1, 4000, 600, 300));
        // Do not DC-block NRZ: long runs of zero are normal for these instruments.
        clock = std::make_shared<dsp::MMClockRecoveryBlock<float>>(filter->output_stream, 4000 / baud, 0.000025f, 0.5f, 0.01f, 0.002f);
    }
    void TransitDemodModule::process()
    {
        filesize = input_data_type == satdump::pipeline::DATA_FILE ? file_source->getFilesize() : 0;
        progress = 0;
        if (input_data_type == satdump::pipeline::DATA_FILE && filesize == 0)
            throw satdump_exception("TRANSIT input baseband is empty");
        if (output_data_type == satdump::pipeline::DATA_FILE)
        {
            d_output_file = d_output_file_hint + ".soft";
            data_out.open(d_output_file, std::ios::binary);
            if (!data_out) throw satdump_exception("Cannot create TRANSIT soft symbols");
        }
        BaseDemodModule::start();
        fine_tune->start(); rf_spectrum->start();
        rf_demod->start(); pam->start(); mixer->start(); pcm_resampler->start(); pcm_spectrum->start(); fsk_demod->start(); filter->start(); clock->start();
        std::vector<int8_t> soft;
        while (demod_should_run())
        {
            int n = clock->output_stream->read();
            if (n <= 0) { clock->output_stream->flush(); continue; }
            soft.resize(n);
            for (int i = 0; i < n; ++i) soft[i] = clamp(clock->output_stream->readBuf[i] * 90);
            {
                std::lock_guard<std::mutex> lock(preview_mutex);
                auto before = preview.subframes;
                for (int8_t v : soft) preview.push(v);
                if (preview.subframes != before) last_frame_received = std::chrono::steady_clock::now();
            }
            clock->output_stream->flush();
            if (output_data_type == satdump::pipeline::DATA_FILE)
            {
                data_out.write(reinterpret_cast<char *>(soft.data()), n);
                data_out.flush(); // At 391 bytes/s, buffering otherwise hides data for ~20 seconds.
                saved = bool(data_out);
                if (!data_out) { write_error = true; logger->error("TRANSIT: writing soft symbols failed"); break; }
            }
            else output_fifo->write(reinterpret_cast<uint8_t *>(soft.data()), n);
            if (input_data_type == satdump::pipeline::DATA_FILE) progress = file_source->getPosition();
        }
        {
            std::lock_guard<std::mutex> lock(preview_mutex);
            preview.finish();
        }
        if (input_data_type == satdump::pipeline::DATA_FILE) stop();
        // process() owns the file; stop() may run on another thread.
        if (data_out.is_open())
        {
            data_out.close();
            if (!data_out) { write_error = true; saved = false; logger->error("TRANSIT: closing soft symbols failed"); }
        }
    }
    nlohmann::json TransitDemodModule::getModuleStats()
    {
        std::lock_guard<std::mutex> lock(preview_mutex);
        nlohmann::json v = {{"symbols", preview.symbols}, {"subframes", preview.subframes},
            {"synced", preview.synced()}, {"frequency_shift_hz", d_frequency_shift},
            {"saved", saved.load()}, {"write_error", write_error.load()}};
        if (pam)
        {
            auto p = pam->snapshot();
            v["pam_frames"] = p.frames; v["pam_synced"] = p.synced;
            v["pam_rejected"] = p.rejected; v["pam_timing_discontinuities"] = p.discontinuities;
            v["pam_write_error"] = p.write_error;
            v["write_error"] = write_error.load() || p.write_error;
        }
        if (rf_spectrum)
        {
            auto peak = rf_spectrum->peak();
            v["carrier_offset_hz"] = peak.ready ? nlohmann::json(peak.hz) : nlohmann::json(nullptr);
            v["carrier_candidate"] = peak.visible;
        }
        if (input_data_type == satdump::pipeline::DATA_FILE)
            v["progress"] = filesize ? double(progress.load()) / filesize.load() : 0.0;
        return v;
    }
    void TransitDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), nullptr, window ? 0 : NOWINDOW_FLAGS);
        auto peak = rf_spectrum ? rf_spectrum->peak() : TuningSpectrum::Peak{};
        auto ps = pam ? pam->snapshot() : PamTap::Snapshot{};
        begin_status_row();
        {
            std::lock_guard<std::mutex> lock(preview_mutex);
            const bool fresh = preview.synced() && std::chrono::steady_clock::now() - last_frame_received < std::chrono::seconds(2);
            lamp("RF", peak.visible, "Carrier candidate: peak is >10 dB above the local spectral median. This is not a calibrated SNR."); ImGui::SameLine();
            lamp("SYNC", fresh, "Alternating E240/E25F sync words confirmed at 256-bit spacing. No CRC."); ImGui::SameLine();
            lamp("DATA", fresh && std::chrono::steady_clock::now() - last_frame_received < std::chrono::milliseconds(180), "Flashes when a subframe arrives."); ImGui::SameLine();
            lamp("SAVE", saved && !write_error && !ps.write_error, "Soft-symbol writes flushed to disk; no PAM write error."); ImGui::SameLine();
            lamp("I/O", write_error || ps.write_error, "Output write failed. Check the output path and disk space.", true);
            ImGui::SameLine();
            lamp("PAM", ps.synced, "Long analog sync repeated at the expected period, with calibration segments confirmed.");
            ImGui::SameLine();
            format_lamps(preview.latest, fresh);
            ImGui::SameLine(); ImGui::Text("%llu fr", (unsigned long long)preview.subframes);
            ImGui::SameLine(); frame_values(preview.latest, fresh);
            ImGui::SameLine();
            status_flags(preview.latest, fresh);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(130 * ui_scale);
        bool changed = ImGui::InputFloat("Hz##Fine shift", &fine_shift_hz, 25, 250, "%.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fine frequency shift in Hz (applied immediately)");
        ImGui::SameLine();
        if (ImGui::Button("Center peak") && peak.visible) { fine_shift_hz -= peak.hz; changed = true; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move the strongest carrier candidate to zero. Coarse IQ shift: %ld Hz", d_frequency_shift);
        if (changed && fine_tune)
        {
            fine_shift_hz = std::isfinite(fine_shift_hz) ? std::max(-12000.0f, std::min(12000.0f, fine_shift_hz)) : 0;
            fine_tune->set_freq(final_samplerate, fine_shift_hz);
        }
        end_status_row();
        if (ImGui::BeginTabBar("Tuning"))
        {
            if (ImGui::BeginTabItem("RF tuning"))
            {
                if (rf_spectrum) rf_spectrum->draw(125 * ui_scale, {{"carrier", 0}});
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("PCM tones"))
            {
                if (pcm_spectrum) pcm_spectrum->draw(125 * ui_scale, {{"0", -float(deviation)}, {"1", float(deviation)}});
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("PAM"))
            {
                auto p = pam ? pam->snapshot(true) : PamTap::Snapshot{};
                ImGui::Text("PAM %llu frames | rejected %llu | timing gaps %llu", (unsigned long long)p.frames,
                    (unsigned long long)p.rejected, (unsigned long long)p.discontinuities);
                std::vector<double> t, hz, env;
                for (const auto &v : p.waveform)
                { t.push_back(v.t); hz.push_back(v.valid ? v.hz : std::numeric_limits<double>::quiet_NaN()); env.push_back(v.envelope); }
                if (ImPlot::BeginPlot("PAM subcarrier", ImVec2(-1, 150 * ui_scale)))
                {
                    ImPlot::SetupAxes("Sample time (s)", "Deviation (Hz)");
                    ImPlot::SetupAxisLimits(ImAxis_X1, std::max(0.0, p.time - 13), std::max(13.0, p.time), ImPlotCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -500, 500, ImPlotCond_Once);
                    ImPlot::SetupAxis(ImAxis_Y2, "Envelope (relative)", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_AutoFit);
                    if (!t.empty())
                    {
                        ImPlot::PlotLine("PAM", t.data(), hz.data(), int(t.size()));
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                        ImPlot::PlotLine("Envelope", t.data(), env.data(), int(t.size()));
                    }
                    ImPlot::EndPlot();
                }
                if (p.frames) draw_pam_frame(p.latest);
                else ImGui::TextDisabled("Waiting for two sync plateaus and calibration segments (about 26 s).");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        if (input_data_type == satdump::pipeline::DATA_FILE)
            ImGui::ProgressBar(filesize ? float(progress.load()) / filesize.load() : 0.0f);
        drawStopButton();
        ImGui::End();
    }
    void TransitDemodModule::stop()
    {
        BaseDemodModule::stop();
        fine_tune->stop(); rf_spectrum->stop();
        rf_demod->stop(); pam->stop(); mixer->stop(); pcm_resampler->stop(); pcm_spectrum->stop(); fsk_demod->stop(); filter->stop(); clock->stop();
        clock->output_stream->stopReader();

    }
}
