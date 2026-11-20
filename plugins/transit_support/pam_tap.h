#pragma once
#include "pam_demod.h"
#include "common/dsp/block.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <mutex>
#include <iomanip>
#include <chrono>

namespace transit
{
    inline nlohmann::json pam_frame_json(const PamFrame &f)
    {
        nlohmann::json levels = nlohmann::json::array(), status = nlohmann::json::array();
        for (int k = 0; k < 34; ++k) levels.push_back(f.coverage[k] >= .8 ? nlohmann::json(f.hz[k]) : nlohmann::json(nullptr));
        for (int k = 0; k < 66; ++k) status.push_back(f.status_valid[k] ? nlohmann::json(f.status_hz[k]) : nlohmann::json(nullptr));
        return {{"kind", "frame"}, {"start_s", f.start}, {"period_s", f.period}, {"following_sync", f.following_sync},
            {"segments_hz", levels}, {"mad_hz", f.spread}, {"valid_fraction", f.coverage}, {"status8_hz_5ms", status}};
    }
    class PamTap : public dsp::Block<float, float>
    {
        PamDemod demod;
        PamDecoder decoder;
        std::mutex mutex;
        std::ofstream frames_out, wave_out, segments_out;
        bool failed = false, finished = false;
        uint64_t written_samples = 0;
        std::chrono::steady_clock::time_point last_sample{};
        std::vector<float> wave_group, envelope_group;
        int wave_valid = 0;
        void work() override
        {
            int n = input_stream->read();
            if (n <= 0) { input_stream->flush(); return; }
            {
                std::lock_guard<std::mutex> lock(mutex);
                last_sample = std::chrono::steady_clock::now();
                for (int i = 0; i < n; ++i)
                {
                    float x = input_stream->readBuf[i];
                    demod.push(x); output_stream->writeBuf[i] = x;
                }
            }
            input_stream->flush(); output_stream->swap(n);
        }
    public:
        struct Snapshot
        {
            bool synced = false, write_error = false;
            uint64_t frames = 0, rejected = 0, discontinuities = 0;
            double time = 0;
            PamFrame latest;
            std::vector<PamSample> waveform;
        };
        PamTap(std::shared_ptr<dsp::stream<float>> input, double center, double baud, const std::string &prefix)
            : Block(input), demod(center), decoder(baud)
        {
            frames_out.open(prefix + ".pam.jsonl"); wave_out.open(prefix + ".pam_waveform.csv"); segments_out.open(prefix + ".pam_segments.csv");
            if (!frames_out || !wave_out || !segments_out) throw satdump_exception("Cannot create TRANSIT PAM outputs");
            frames_out << nlohmann::json({{"kind", "metadata"}, {"version", 1}, {"source", "analog_subcarrier"},
                {"center_hz", center}, {"baudrate", baud}, {"segment_bits", 128}, {"sync_bits", 512}, {"frame_bits", 4864},
                {"time_basis", "RF discriminator samples, local FIR delay compensated; upstream resampler delay not compensated"},
                {"reference", "TRANSIT_5E5_PAM.md (2026-10-26), supplied measurements; channel meanings provisional"}}).dump() << '\n';
            wave_out << "sample_time_s,deviation_hz,envelope_relative,valid\n";
            segments_out << "frame_start_s,following_sync,segment,deviation_hz,mad_hz,valid_fraction\n";
            frames_out.flush(); wave_out.flush(); segments_out.flush();
            decoder.on_frame = [this](const PamFrame &f)
            {
                frames_out << pam_frame_json(f).dump() << '\n';
                for (int k = 0; k < 34; ++k)
                {
                    segments_out << std::setprecision(10) << f.start << ',' << f.following_sync << ',' << k + 1 << ',';
                    if (f.coverage[k] >= .8) segments_out << f.hz[k];
                    segments_out << ',' << f.spread[k] << ',' << f.coverage[k] << '\n';
                }
                frames_out.flush(); segments_out.flush();
                failed |= !frames_out || !segments_out;
            };
            demod.on_sample = [this](PamSample s)
            {
                decoder.push(s);
                wave_group.push_back(s.hz); envelope_group.push_back(s.envelope); wave_valid += s.valid;
                if (wave_group.size() == 5)
                {
                    wave_out << std::setprecision(10) << s.t - .002 << ',' << pam_median(wave_group) << ','
                             << pam_median(envelope_group) << ',' << (wave_valid >= 4) << '\n';
                    wave_group.clear(); envelope_group.clear(); wave_valid = 0;
                }
                if (++written_samples % 250 == 0) { wave_out.flush(); failed |= !wave_out; }
            };
        }
        Snapshot snapshot(bool include_waveform = false)
        {
            std::lock_guard<std::mutex> lock(mutex);
            Snapshot s; s.synced = decoder.synced() && !finished && std::chrono::steady_clock::now() - last_sample < std::chrono::seconds(2); s.write_error = failed;
            s.frames = decoder.frames; s.rejected = decoder.rejected; s.discontinuities = decoder.discontinuities;
            s.time = decoder.time; s.latest = decoder.latest;
            if (include_waveform)
            {
                int n = 0;
                for (const auto &p : decoder.samples())
                    if (p.t >= decoder.time - decoder.period() - .5 && ++n % 5 == 0) s.waveform.push_back(p);
            }
            return s;
        }
        void stop() override
        {
            Block::stop();
            std::lock_guard<std::mutex> lock(mutex);
            if (finished) return;
            decoder.finish();
            frames_out << nlohmann::json({{"kind", "end"}, {"frames", decoder.frames}, {"rejected", decoder.rejected},
                {"timing_discontinuities", decoder.discontinuities}, {"sample_time_s", decoder.time}}).dump() << '\n';
            frames_out.close(); wave_out.close(); segments_out.close();
            failed |= !frames_out || !wave_out || !segments_out;
            finished = true;
        }
    };
}
