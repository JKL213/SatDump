# Recorder frontend regression test

This standalone test uses an existing Linux build of `satdump_core` and compiles
the changed DC filter and RTL-SDR source directly. It does not open an SDR: the
RTL-SDR lifecycle and configuration API is replaced by test functions. It does not change the existing
build directory or require changes to the project's CMake configuration.

From the repository root, with the usual SatDump and librtlsdr build dependencies:

```sh
test_build=$(mktemp -d /tmp/satdump-recorder-test.XXXXXX)
c++ -std=c++17 -O2 -DENABLE_I18N=1 -DSOURCE_PATH_SIZE=0 \
  -I src-core -I src-interface -I plugins/sdr_sources/rtlsdr_sdr_support \
  tests/recorder_frontend_test.cpp \
  src-core/common/dsp/utils/correct_iq.cpp \
  plugins/sdr_sources/rtlsdr_sdr_support/rtlsdr_sdr.cpp \
  -L "$PWD/build" -Wl,-rpath,"$PWD/build" \
  -lsatdump_core -lrtlsdr -lvolk -pthread \
  -o "$test_build/recorder_frontend_test"
timeout 45s "$test_build/recorder_frontend_test"
```

Coverage:

- Native offset tuning: enable/disable, settings persistence, unsupported tuner,
  direct sampling, driver errors and recovery; no driver calls while stopped or
  redundant retunes when the setting is unchanged. Retry after partial hardware
  failure even if the driver's cached state already reflects the requested value.
- Simulated E4000 stop/start: reapply PPM and offset on each opened device, keep
  the requested RF centre, handle missing devices and failed opens/sample rates.
  The mock verifies application behaviour, not the tuner's physical LO or DDC.
- Marker frequency: module defaults, pipeline overrides, zero and both shift
  signs, verified against the actual frequency-shifting DSP block.
- Shared DC correction: DC plus a 1 kHz signal at 48 kS/s, 250 kS/s, 2.4 and 3.2 MS/s;
  verify DC rejection, signal preservation and identical samples on FFT,
  recording and live outputs. Require residual DC below 1e-6 after settling.
  Repeat with fresh blocks to cover restart state. Also check the real-valued
  filter and rejection of invalid/non-finite coefficients.

Implementation notes:

- Use `rtlsdr_set_offset_tuning` for the E4000. It sets the tuner offset, adjusts
  its filter bandwidth and compensates in the RTL2832 DDC. Do not add another
  application-level frequency shift. See the
  [librtlsdr implementation](https://github.com/osmocom/rtl-sdr/blob/master/src/librtlsdr.c).
- DC correction uses `average += alpha * (sample - average)` with double-precision
  state. Float state can stall above zero at MHz sample rates. This is DC removal,
  not a gain/phase IQ imbalance calibration.

Hardware/UI checks after building the application:

1. With an E4000 receiver, enable **Offset Tuning** in the RTL-SDR controls. Verify
   that the requested RF frequency and signal position remain unchanged while
   the tuner-centre disturbance is suppressed. Check the E4000 identification,
   changing gain with offset enabled, and stop/start with a nonzero PPM setting.
   On R820T/R828D devices the unsupported offset control should be disabled.
2. Stop the source, enable **DC correction** and restart.
   Verify the spectrum and a recording; repeat with recorder decimation enabled.
   Stop/start again and reload the application to check the saved settings.
3. Set positive/negative pipeline **Frequency Shift** values and hover the VFO
   arrow. Check the preview and running-pipeline marker. The marker represents
   the configured static shift, not demodulator AFC or dynamic Doppler tracking.
4. Check simultaneous autotrack VFOs and deletion of one VFO, including duplicate
   display names. Other markers must remain visible.
