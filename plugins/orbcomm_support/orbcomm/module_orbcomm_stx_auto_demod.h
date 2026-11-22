#pragma once

#include "common/audio/audio_sink.h"
#include "common/dsp/path/splitter.h"
#include "pipeline/modules/demod/module_demod_base.h"
#include "stx_demod_hier.h"
#include <atomic>
#include <cmath>
#include <mutex>

namespace orbcomm
{
    class OrbcommSTXAutoDemodModule : public satdump::pipeline::demod::BaseDemodModule
    {
    protected:
        const double d_center_frequency;

        std::shared_ptr<dsp::SplitterBlock> splitter;

        struct STXLink
        {
            std::string id;
            std::shared_ptr<STXDemod> demod;
            time_t last_ping;
        };
        std::mutex stx_link_lst_mtx;
        std::map<double, STXLink> stx_link_lst;

        std::mutex audio_mtx;
        std::atomic<bool> play_audio{false};
        std::atomic<bool> enable_audio{false};
        std::atomic<double> audio_freq{0};
        std::shared_ptr<audio::AudioSink> audio_sink;
        int16_t *audio_buffer = nullptr;

        void add_stx_link(double frequency)
        {
            double offset = d_center_frequency - frequency;
            if (!std::isfinite(frequency) || std::abs(offset) > d_samplerate / 2.0)
                return;

            stx_link_lst_mtx.lock();
            if (stx_link_lst.count(frequency) > 0)
            {
                stx_link_lst[frequency].last_ping = time(0);
            }
            else
            {
                std::string id = std::to_string(frequency);
                splitter->add_vfo(id, d_samplerate, offset);
                STXLink link;
                link.id = id;
                link.demod = std::make_shared<STXDemod>(splitter->get_vfo_output(id), frm_callback, d_samplerate, true,
                                                        [this, frequency](float *buf, int n)
                                                        {
                                                            if (!(enable_audio.load() && play_audio.load() && audio_freq.load() == frequency))
                                                                return;
                                                            std::lock_guard<std::mutex> lock(audio_mtx);
                                                            if (!(enable_audio.load() && play_audio.load() && audio_freq.load() == frequency) || !audio_sink || !audio_buffer)
                                                                return;
                                                            int count = std::min(n, dsp::STREAM_BUFFER_SIZE);
                                                            for (int i = 0; i < count; i++)
                                                                audio_buffer[i] = std::max(-1.0f, std::min(1.0f, buf[i])) * 32767;
                                                            audio_sink->push_samples(audio_buffer, count);
                                                        });
                link.last_ping = time(0);
                link.demod->start();
                logger->info("Started!");
                splitter->set_vfo_enabled(id, true);
                logger->info("Enabled!");
                stx_link_lst.insert({frequency, link});
                logger->info("Copied!");

                logger->critical("OFFSET %f", offset);
            }
            stx_link_lst_mtx.unlock();
        }

        void del_stx_link(double frequency)
        {
            stx_link_lst_mtx.lock();
            if (stx_link_lst.count(frequency) > 0)
            {
                if (audio_freq.load() == frequency)
                    audio_freq.store(0);
                splitter->set_vfo_enabled(stx_link_lst[frequency].id, false);
                stx_link_lst[frequency].demod->stop();
                splitter->del_vfo(stx_link_lst[frequency].id);
                stx_link_lst.erase(frequency);
            }
            stx_link_lst_mtx.unlock();
        }

        std::mutex freqs_to_push_mtx;
        std::vector<double> freqs_to_push;

    protected:
        std::function<void(uint8_t *, int)> frm_callback;

    public:
        OrbcommSTXAutoDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~OrbcommSTXAutoDemodModule();
        void init();
        void stop();
        void process();

        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace orbcomm