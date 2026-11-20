#include "module_orbcomm_stx_demod.h"
#include "common/dsp/filter/firdes.h"
#include "core/config.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "utils/binary.h"
#include <volk/volk.h>

namespace orbcomm
{
    OrbcommSTXDemodModule::OrbcommSTXDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : BaseDemodModule(input_file, output_file_hint, parameters), stx_deframer(ORBCOMM_STX_FRM_SIZE)
    {
        name = "Orbcomm STX Demodulator";
        show_freq = false;
        play_audio = satdump::satdump_cfg.shouldPlayAudio();

        constellation.d_hscale = 2.0; // 80.0 / 100.0;
        constellation.d_vscale = 0.2; // 20.0 / 100.0;

        MIN_SPS = 1;
        MAX_SPS = 10.0;
    }

    void OrbcommSTXDemodModule::init()
    {
        BaseDemodModule::initb();

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(agc->output_stream, 1);

        // DC Blocking
        iqc = std::make_shared<dsp::CorrectIQBlock<float>>(qua->output_stream);

        // Audio tap
        tap = std::make_shared<dsp::TapBlock<float>>(iqc->output_stream,
                                                     [this](float *buf, int n)
                                                     {
                                                         if (!(enable_audio && play_audio))
                                                             return;
                                                         for (int i = 0; i < n; i++)
                                                             audio_buffer[i] = std::max(-1.0f, std::min(1.0f, buf[i])) * 32767;
                                                         audio_sink->push_samples(audio_buffer, n);
                                                     });

        // Filter
        rrc = std::make_shared<dsp::FIRBlock<float>>(tap->output_stream, dsp::firdes::root_raised_cosine(1.0, final_samplerate, d_symbolrate, 0.4, 31));

        // Recovery
        float d_clock_gain_omega = 0.25 * 0.175 * 0.175;
        float d_clock_mu = 0.5f;
        float d_clock_gain_mu = 0.175;
        float d_clock_omega_relative_limit = 0.005f;
        rec = std::make_shared<dsp::MMClockRecoveryBlock<float>>(rrc->output_stream, final_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);
    }

    OrbcommSTXDemodModule::~OrbcommSTXDemodModule()
    {
        if (audio_buffer != nullptr)
            delete[] audio_buffer;
    }

    void OrbcommSTXDemodModule::process()
    {
        if (input_data_type == satdump::pipeline::DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (output_data_type == satdump::pipeline::DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
            d_output_file = d_output_file_hint + ".frm";
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".frm");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        if (input_data_type != satdump::pipeline::DATA_FILE && audio::has_sink())
        {
            enable_audio = true;
            audio_buffer = new int16_t[dsp::STREAM_BUFFER_SIZE];
            audio_sink = audio::get_default_sink();
            audio_sink->set_samplerate(final_samplerate);
            audio_sink->start();
        }

        // Start
        BaseDemodModule::start();
        qua->start();
        iqc->start();
        tap->start();
        rrc->start();
        rec->start();

        uint8_t *bits_buf = new uint8_t[d_buffer_size * 2];
        uint8_t *frames = new uint8_t[d_buffer_size * 2];

        int dat_size = 0;
        while (demod_should_run())
        {
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
            {
                rec->output_stream->flush();
                continue;
            }

            // Into const
            constellation.pushFloatAndGaussian(rec->output_stream->readBuf, rec->output_stream->getDataSize());

            // Estimate SNR
            snr_estimator.update((complex_t *)rec->output_stream->readBuf, dat_size / 2);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            for (int i = 0; i < dat_size; i++)
                bits_buf[i] = rec->output_stream->readBuf[i] > 0;

            rec->output_stream->flush();

            int framen = stx_deframer.work(bits_buf, dat_size, frames);

            for (int i = 0; i < framen; i++)
                for (int y = 0; y < (ORBCOMM_STX_FRM_SIZE / 8); y++)
                    frames[i * (ORBCOMM_STX_FRM_SIZE / 8) + y] = satdump::reverseBits(frames[i * (ORBCOMM_STX_FRM_SIZE / 8) + y]);

            if (framen > 0)
            {
                if (output_data_type == satdump::pipeline::DATA_FILE)
                    data_out.write((char *)frames, framen * (ORBCOMM_STX_FRM_SIZE / 8));
                else
                    output_fifo->write((uint8_t *)frames, framen * (ORBCOMM_STX_FRM_SIZE / 8));
            }

            if (input_data_type == satdump::pipeline::DATA_FILE)
                progress = file_source->getPosition();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        delete[] bits_buf;
        delete[] frames;

        if (enable_audio)
        {
            enable_audio = false;
            audio_sink->stop();
        }

        logger->info("Demodulation finished");

        if (input_data_type == satdump::pipeline::DATA_FILE)
            stop();
    }

    void OrbcommSTXDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        qua->stop();
        iqc->stop();
        tap->stop();
        rrc->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == satdump::pipeline::DATA_FILE)
            data_out.close();
    }

    void OrbcommSTXDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            // Show SNR information
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
            snr_plot.draw(snr, peak_snr);
            if (!d_is_streaming_input)
                if (ImGui::Checkbox("Show FFT", &show_fft))
                    fft_splitter->set_enabled("fft", show_fft);

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (stx_deframer.getState() == stx_deframer.STATE_NOSYNC)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else if (stx_deframer.getState() == stx_deframer.STATE_SYNCING)
                    ImGui::TextColored(style::theme.orange, "SYNCING");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");
            }

            if (enable_audio)
            {
                const char *btn_icon, *label;
                ImVec4 color;
                if (play_audio)
                {
                    color = style::theme.green.Value;
                    btn_icon = u8"\uF028##stxaudio";
                    label = "Audio Playing";
                }
                else
                {
                    color = style::theme.red.Value;
                    btn_icon = u8"\uF026##stxaudio";
                    label = "Audio Muted";
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                if (ImGui::Button(btn_icon))
                    play_audio = !play_audio;
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::TextUnformatted(label);
            }
        }
        ImGui::EndGroup();

        if (!d_is_streaming_input)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        drawStopButton();

        ImGui::End();

        drawFFT();
    }

    std::string OrbcommSTXDemodModule::getID() { return "orbcomm_stx_demod"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> OrbcommSTXDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<OrbcommSTXDemodModule>(input_file, output_file_hint, parameters);
    }
} // namespace orbcomm