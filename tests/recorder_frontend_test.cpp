// Standalone regression test; see recorder_frontend_test.md for build instructions.
#include "common/dsp/path/splitter.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include "common/dsp/utils/correct_iq.h"
#include "common/dsp/utils/freq_shift.h"
#include "dsp/utils/correct_iq.h"
#include "recorder/pipeline_tuning.h"
#include "rtlsdr_sdr.h"
#include <cmath>
#include <complex>
#include <condition_variable>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>

// Exercise driver success/rejection without opening or changing an SDR.
static int offset_calls = 0, offset_state = 0, offset_result = 0;
static bool offset_partial_failure = false, offset_ignore_write = false;
extern "C" int rtlsdr_set_offset_tuning(rtlsdr_dev_t *, int enabled)
{
    offset_calls++;
    if ((offset_result == 0 && !offset_ignore_write) || offset_partial_failure)
        offset_state = enabled;
    return offset_result;
}
extern "C" int rtlsdr_get_offset_tuning(rtlsdr_dev_t *) { return offset_state; }

// Fake E4000 lifecycle: no USB calls, even when testing stop/start and failures.
struct rtlsdr_dev {};
static rtlsdr_dev fake_device;
static int device_index = 0, open_result = 0, rate_result = 0;
static int open_calls = 0, close_calls = 0, ppm_calls = 0, device_ppm = 0, ppm_result = 0;
static uint32_t device_frequency = 0;
static std::mutex rx_mutex;
static std::condition_variable rx_condition;
static bool rx_cancelled = false;
extern "C" int rtlsdr_get_index_by_serial(const char *) { return device_index; }
extern "C" int rtlsdr_open(rtlsdr_dev_t **device, uint32_t)
{
    open_calls++;
    if (open_result != 0)
        return open_result;
    *device = &fake_device;
    device_ppm = offset_state = 0;
    return 0;
}
extern "C" int rtlsdr_close(rtlsdr_dev_t *) { close_calls++; return 0; }
extern "C" rtlsdr_tuner rtlsdr_get_tuner_type(rtlsdr_dev_t *) { return RTLSDR_TUNER_E4000; }
extern "C" int rtlsdr_get_tuner_gains(rtlsdr_dev_t *, int *gains)
{
    if (gains)
    {
        gains[0] = -10;
        gains[1] = 15;
        gains[2] = 490;
    }
    return 3;
}
extern "C" int rtlsdr_set_sample_rate(rtlsdr_dev_t *, uint32_t) { return rate_result; }
extern "C" int rtlsdr_get_freq_correction(rtlsdr_dev_t *) { return device_ppm; }
extern "C" int rtlsdr_set_freq_correction(rtlsdr_dev_t *, int ppm)
{
    ppm_calls++;
    if (ppm_result != 0)
        return ppm_result;
    device_ppm = ppm;
    return 0;
}
extern "C" int rtlsdr_set_center_freq(rtlsdr_dev_t *, uint32_t frequency) { device_frequency = frequency; return 0; }
extern "C" int rtlsdr_set_agc_mode(rtlsdr_dev_t *, int) { return 0; }
extern "C" int rtlsdr_set_tuner_gain_mode(rtlsdr_dev_t *, int) { return 0; }
extern "C" int rtlsdr_set_tuner_gain(rtlsdr_dev_t *, int) { return 0; }
extern "C" int rtlsdr_set_bias_tee(rtlsdr_dev_t *, int) { return 0; }
extern "C" int rtlsdr_reset_buffer(rtlsdr_dev_t *)
{
    std::lock_guard<std::mutex> lock(rx_mutex);
    rx_cancelled = false;
    return 0;
}
extern "C" int rtlsdr_cancel_async(rtlsdr_dev_t *)
{
    std::lock_guard<std::mutex> lock(rx_mutex);
    rx_cancelled = true;
    rx_condition.notify_all();
    return 0;
}
extern "C" int rtlsdr_read_async(rtlsdr_dev_t *, rtlsdr_read_async_cb_t, void *, uint32_t, uint32_t)
{
    std::unique_lock<std::mutex> lock(rx_mutex);
    rx_condition.wait(lock, [] { return rx_cancelled; });
    return 0;
}

static void check(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

class TestRtlSource : public RtlSdrSource
{
public:
    TestRtlSource() : RtlSdrSource({"rtlsdr", "test", "test"}) { rtlsdr_dev_obj = nullptr; }
    ~TestRtlSource() { is_started = false; }
    void running(bool value) { is_started = value; }
    void apply(bool value)
    {
        offset_tuning = value;
        set_offset_tuning();
    }
    bool enabled() const { return offset_tuning; }
    bool has_error() const { return !offset_tuning_error.empty(); }
};

static void test_offset_tuning()
{
    TestRtlSource source;
    source.set_settings({{"offset_tuning", true}});
    check(source.get_settings().at("offset_tuning") == true, "offset setting was not preserved");
    check(offset_calls == 0, "driver called while source stopped");
    source.running(true);
    source.apply(true);
    check(offset_calls == 1 && offset_state == 1 && source.enabled(), "enabling offset tuning failed");
    source.apply(true);
    check(offset_calls == 1, "unchanged offset setting retuned the E4000");
    source.apply(false);
    check(offset_calls == 2 && offset_state == 0 && !source.enabled(), "disabling offset tuning failed");
    source.apply(false);
    check(offset_calls == 2, "unneeded disable call on a tuner without offset tuning");
    for (int error : {-2, -3, -1})
    {
        offset_result = error;
        source.apply(true);
        check(!source.enabled() && source.has_error(), "rejected offset tuning shown as enabled");
    }
    offset_result = 0;
    source.apply(true);
    check(source.enabled() && !source.has_error(), "offset tuning did not recover after failure");
    offset_result = -1;
    source.apply(false);
    check(source.enabled() && source.has_error(), "failed disable did not preserve actual state");
    offset_result = 0;
    source.apply(false);

    offset_result = -1;
    offset_partial_failure = true;
    source.apply(true);
    check(source.has_error() && source.enabled(), "partial hardware failure was hidden");
    const int calls_before_retry = offset_calls;
    offset_result = 0;
    offset_partial_failure = false;
    source.apply(true);
    check(offset_calls == calls_before_retry + 1 && !source.has_error(), "cached state prevented retry after a partial hardware failure");
    source.apply(false);

    offset_ignore_write = true;
    source.apply(true);
    check(source.has_error() && !source.enabled(), "successful return with unchanged state was accepted");
    offset_ignore_write = false;
    source.apply(false);
    offset_state = -1;
    const int calls_before_read_error = offset_calls;
    source.apply(true);
    check(source.has_error() && offset_calls == calls_before_read_error, "failed state query was ignored");
    offset_state = 0;
    source.apply(false);
    source.running(false);
    source.set_settings({});
    check(!source.enabled(), "legacy settings unexpectedly enabled offset tuning");
    std::cout << "PASS native offset setting, persistence, rejection and recovery\n";
}

static void test_e4000_restart()
{
    RtlSdrSource source({"rtlsdr", "fake E4000", "fake"});
    source.open();
    source.set_frequency(137100000);
    source.set_samplerate(250000);
    source.set_settings({{"offset_tuning", true}, {"ppm_correction", 7}});
    const int initial_offset_calls = offset_calls;
    for (int session = 1; session <= 2; session++)
    {
        source.start();
        check(offset_state == 1 && offset_calls == initial_offset_calls + session, "offset was not reapplied after reopening");
        check(device_ppm == 7 && ppm_calls == session, "PPM correction was not reapplied after reopening");
        check(device_frequency == 137100000, "offset tuning changed requested RF centre");
        source.set_settings(source.get_settings());
        check(offset_calls == initial_offset_calls + session, "unchanged settings caused a tuner retune");
        source.start();
        check(open_calls == session, "starting a running source reopened the device");
        if (session == 2)
        {
            auto settings = source.get_settings();
            settings["ppm_correction"] = 9;
            ppm_result = -1;
            source.set_settings(settings);
            check(device_ppm == 7, "failed PPM write changed the device");
            const int calls_before_retry = ppm_calls;
            ppm_result = 0;
            source.set_settings(settings);
            check(device_ppm == 9 && ppm_calls == calls_before_retry + 1, "failed PPM value was cached as successful");
        }
        source.stop();
        source.stop();
        check(close_calls == session, "stop did not close exactly once");
    }
    const int initial_open_calls = open_calls;
    for (int error : {-1, -2, -3})
    {
        device_index = error;
        bool rejected = false;
        try { source.start(); }
        catch (const std::runtime_error &) { rejected = true; }
        check(rejected && open_calls == initial_open_calls, "missing device reached the hardware API");
    }
    device_index = 0;
    open_result = -1;
    bool rejected = false;
    try { source.start(); }
    catch (const std::runtime_error &) { rejected = true; }
    check(rejected && close_calls == 2, "failed open did not leave the source stopped");
    open_result = 0;
    rate_result = -1;
    rejected = false;
    try { source.start(); }
    catch (const std::runtime_error &) { rejected = true; }
    check(rejected && close_calls == 3, "samplerate failure leaked the device");
    source.stop();
    check(close_calls == 3, "samplerate failure closed the device twice");
    rate_result = 0;
    std::cout << "PASS E4000 restart, PPM/offset persistence, RF centre and startup failure cleanup\n";
}

static void test_dc_correction(int samplerate)
{
    auto input = std::make_shared<dsp::stream<complex_t>>();
    dsp::CorrectIQBlock<complex_t> correction(input, dsp::CorrectIQBlock<complex_t>::alpha_from_samplerate(samplerate));
    dsp::SplitterBlock splitter(correction.output_stream);
    splitter.add_output("record");
    splitter.add_output("live");
    splitter.set_enabled("record", true);
    splitter.set_enabled("live", true);
    std::shared_ptr<dsp::stream<complex_t>> outputs[] = {splitter.output_stream, splitter.get_output("record"), splitter.get_output("live")};
    correction.start();
    splitter.start();

    std::complex<double> mean[3] = {}, tone[3] = {};
    int measured = 0;
    bool equal = true;
    for (int first = 0; first < samplerate / 2;)
    {
        int count = std::min(4096, samplerate / 2 - first);
        for (int i = 0; i < count; i++)
        {
            double phase = 2 * M_PI * 1000 * (first + i) / samplerate;
            input->writeBuf[i] = complex_t(0.3f + 0.2f * cos(phase), -0.2f + 0.2f * sin(phase));
        }
        input->swap(count);
        for (auto &output : outputs)
            check(output->read() == count, "DC correction changed the sample count");
        for (int i = 0; i < count; i++)
        {
            for (int port = 1; port < 3; port++)
                equal &= outputs[port]->readBuf[i].real == outputs[0]->readBuf[i].real && outputs[port]->readBuf[i].imag == outputs[0]->readBuf[i].imag;
            if (first + i < samplerate / 4)
                continue;
            double phase = 2 * M_PI * 1000 * (first + i) / samplerate;
            for (int port = 0; port < 3; port++)
            {
                std::complex<double> value(outputs[port]->readBuf[i].real, outputs[port]->readBuf[i].imag);
                mean[port] += value;
                tone[port] += value * std::polar(1.0, -phase);
            }
            measured++;
        }
        for (auto &output : outputs)
            output->flush();
        first += count;
    }
    splitter.stop();
    correction.stop();
    check(equal, "FFT, recording and live received different corrected samples");
    for (int port = 0; port < 3; port++)
    {
        check(std::abs(mean[port] / (double)measured) < 0.000001, "residual DC exceeds threshold");
        check(std::abs(std::abs(tone[port] / (double)measured) - 0.2) < 0.002, "DC correction damaged the off-centre signal");
    }
    std::cout << "PASS DC + 1 kHz tone at " << samplerate << " sps; residual DC " << std::abs(mean[0] / (double)measured) << "\n";
}

static void test_real_dc_correction()
{
    auto input = std::make_shared<dsp::stream<float>>();
    for (double alpha : {0.0, -0.1, 1.1, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
    {
        bool rejected = false;
        try { dsp::CorrectIQBlock<float> correction(input, alpha); }
        catch (const std::runtime_error &) { rejected = true; }
        check(rejected, "invalid DC filter coefficient was accepted");
    }
    check(std::abs(dsp::CorrectIQBlock<complex_t>::alpha_from_samplerate(250000.0) - (50.0 / 250000.0)) < 1e-15, "default alpha_from_samplerate mismatch");
    check(std::abs(dsp::CorrectIQBlock<complex_t>::alpha_from_samplerate(1000.0, 0.1) - 0.01) < 1e-15, "custom time constant alpha_from_samplerate mismatch");
    check(dsp::CorrectIQBlock<complex_t>::alpha_from_samplerate(10.0, 0.02) == 1.0, "alpha_from_samplerate did not clamp to 1.0");
    for (double invalid : {0.0, -1000.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
    {
        bool rejected_sr = false, rejected_tau = false;
        try { dsp::CorrectIQBlock<complex_t>::alpha_from_samplerate(invalid, 0.02); }
        catch (const std::runtime_error &) { rejected_sr = true; }
        try { dsp::CorrectIQBlock<complex_t>::alpha_from_samplerate(250000.0, invalid); }
        catch (const std::runtime_error &) { rejected_tau = true; }
        check(rejected_sr && rejected_tau, "invalid samplerate or time constant was accepted");
    }
    dsp::CorrectIQBlock<float> correction(input, 0.001);
    correction.start();
    float last = 1;
    for (int block = 0; block < 8; block++)
    {
        std::fill(input->writeBuf, input->writeBuf + 4096, 0.3f);
        input->swap(4096);
        check(correction.output_stream->read() == 4096, "real DC filter changed sample count");
        last = correction.output_stream->readBuf[4095];
        correction.output_stream->flush();
    }
    correction.stop();
    check(std::abs(last) < 0.000001f, "real DC estimator stalled before convergence");

    satdump::ndsp::CorrectIQBlock<complex_t> ndsp_correction;
    std::vector<complex_t> ndsp_in(4096, complex_t(0.3f, -0.2f)), ndsp_out(4096);
    for (int block = 0; block < 64; block++)
        check(ndsp_correction.process(ndsp_in.data(), 4096, ndsp_out.data()) == 4096, "ndsp DC filter changed sample count");
    check(std::abs(ndsp_out.back().real) < 0.000001f && std::abs(ndsp_out.back().imag) < 0.000001f, "ndsp DC estimator stalled before convergence");
    std::cout << "PASS real DC correction and invalid coefficient rejection\n";
}

static void test_pipeline_marker()
{
    satdump::pipeline::Pipeline pipeline;
    check(satdump::recorder::pipeline_frequency_shift(pipeline, {}) == 0, "empty pipeline has a shift");
    pipeline.steps.push_back({"baseband", "test", {{"freq_shift", 25000}}, ""});
    pipeline.live_cfg.normal_live = {0};
    check(satdump::recorder::pipeline_frequency_shift(pipeline, {}) == 25000, "module shift default ignored");
    check(satdump::recorder::pipeline_frequency_shift(pipeline, {{"freq_shift", -25000}}) == -25000, "negative pipeline override ignored");
    check(satdump::recorder::pipeline_frequency_shift(pipeline, {{"freq_shift", 0}}) == 0, "zero pipeline override ignored");
    for (double shift : {-25000.0, 0.0, 25000.0})
    {
        auto input = std::make_shared<dsp::stream<complex_t>>();
        dsp::FreqShiftBlock mixer(input, 250000, shift);
        mixer.start();
        double target = satdump::recorder::pipeline_frequency(137100000, shift);
        for (int i = 0; i < 4096; i++)
        {
            double phase = 2 * M_PI * (target - 137100000) * i / 250000;
            input->writeBuf[i] = complex_t(cos(phase), sin(phase));
        }
        input->swap(4096);
        int count = mixer.output_stream->read();
        double error = 0;
        for (int i = 0; i < count; i++)
            error += std::abs(std::complex<double>(mixer.output_stream->readBuf[i].real - 1, mixer.output_stream->readBuf[i].imag));
        mixer.output_stream->flush();
        mixer.stop();
        check(count == 4096 && error / count < 0.001, "marker disagrees with actual frequency mixer");
    }
    std::cout << "PASS pipeline defaults/overrides and marker agreement with the DSP mixer for both signs\n";
}

static void test_source_dc_blocking_settings()
{
    nlohmann::json cfg = {{"dc_blocking", true}};
    check(!satdump::recorder::load_source_dc_blocking(cfg, "HydraSDR One"), "legacy global dc_blocking enabled DC filter for an unconfigured source");
    satdump::recorder::save_source_dc_blocking(cfg, "RTL-SDR E4000", true);
    check(!cfg.contains("dc_blocking"), "legacy global dc_blocking key was written back");
    check(satdump::recorder::load_source_dc_blocking(cfg, "RTL-SDR E4000"), "per-source dc_blocking was not saved for E4000");
    check(!satdump::recorder::load_source_dc_blocking(cfg, "HydraSDR One"), "E4000 dc_blocking leaked to HydraSDR");
    satdump::recorder::save_source_dc_blocking(cfg, "HydraSDR One", false);
    check(satdump::recorder::load_source_dc_blocking(cfg, "RTL-SDR E4000"), "saving HydraSDR overwrote E4000 dc_blocking");
    check(!satdump::recorder::load_source_dc_blocking(cfg, "HydraSDR One"), "HydraSDR dc_blocking did not stay disabled");
    std::cout << "PASS per-source dc_blocking persistence and legacy global key tolerance\n";
}

static void test_pll_frequency_acquisition()
{
    auto input = std::make_shared<dsp::stream<complex_t>>();
    const float target_omega = 0.25f;
    dsp::PLLCarrierTrackingBlock pll(input, 0.002f, 0.5f, -0.5f);
    pll.acquireFrequency(512);
    pll.start();

    // First 512 samples are zero (silence), next 3584 samples carry a tone at target_omega.
    // Acquisition must skip the silence, seed d_freq within the first buffer, and lock before buffer end.
    for (int i = 0; i < 4096; i++)
    {
        if (i < 512)
            input->writeBuf[i] = complex_t(0.0f, 0.0f);
        else
        {
            float phase = target_omega * (i - 512);
            input->writeBuf[i] = complex_t(std::cos(phase), std::sin(phase));
        }
    }
    input->swap(4096);
    check(pll.output_stream->read() == 4096, "PLL changed sample count");
    pll.output_stream->flush();
    pll.stop();
    check(std::abs(pll.getFreq() - target_omega) < 0.005f, "PLL did not acquire carrier frequency on the first buffer");
    std::cout << "PASS PLL coarse frequency acquisition on first buffer after initial silence\n";
}

int main()
{
    logger = std::make_shared<slog::Logger>();
    dsp::STREAM_BUFFER_SIZE = 8192;
    try
    {
        test_offset_tuning();
        test_e4000_restart();
        test_real_dc_correction();
        test_pipeline_marker();
        test_source_dc_blocking_settings();
        test_pll_frequency_acquisition();
        for (int repetition = 0; repetition < 2; repetition++)
            for (int samplerate : {48000, 250000, 2400000, 3200000})
                test_dc_correction(samplerate);
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
