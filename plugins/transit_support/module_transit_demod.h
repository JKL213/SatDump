#pragma once
#include "protocol.h"
#include "tuning_spectrum.h"
#include "pam_tap.h"
#include <chrono>
#include <mutex>
#include "pipeline/modules/demod/module_demod_base.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"

namespace transit
{
    class SubcarrierMixer : public dsp::Block<float, complex_t>
    {
        double phase = 0, step;
        void work() override;
    public:
        SubcarrierMixer(std::shared_ptr<dsp::stream<float>> input, double rate, double center);
    };

    class TransitDemodModule : public satdump::pipeline::demod::BaseDemodModule
    {
        std::shared_ptr<dsp::QuadratureDemodBlock> rf_demod, fsk_demod;
        std::shared_ptr<SubcarrierMixer> mixer;
        std::shared_ptr<dsp::SmartResamplerBlock<complex_t>> pcm_resampler;
        std::shared_ptr<dsp::FIRBlock<float>> filter;
        std::shared_ptr<dsp::MMClockRecoveryBlock<float>> clock;
        double baud, center, deviation, pam_center;
        std::shared_ptr<PamTap> pam;
        std::shared_ptr<dsp::FreqShiftBlock> fine_tune;
        std::unique_ptr<TuningSpectrum> rf_spectrum, pcm_spectrum;
        float fine_shift_hz = 0;
        std::atomic<bool> saved{false}, write_error{false};
        std::chrono::steady_clock::time_point last_frame_received{};
        std::mutex preview_mutex;
        LivePreview preview;
    public:
        TransitDemodModule(std::string, std::string, nlohmann::json);
        void init() override;
        void process() override;
        void stop() override;
        void drawUI(bool window) override;
        nlohmann::json getModuleStats() override;
        static std::string getID() { return "transit_pm_fsk_demod"; }
        std::string getIDM() override { return getID(); }
        static nlohmann::json getParams() { return {}; }
        static std::shared_ptr<ProcessingModule> getInstance(std::string i, std::string o, nlohmann::json p)
        { return std::make_shared<TransitDemodModule>(i, o, p); }
    };
}
