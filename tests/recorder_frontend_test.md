# Recorder frontend regression test

This test exercises the recorder frontend, DC filter and RTL-SDR source without
opening hardware: the RTL-SDR lifecycle and configuration API is replaced by
test functions.

From the repository root:

```sh
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
cmake -B build -DSATDUMP_BUILD_TESTS=ON
cmake --build build -j$(nproc) --target recorder_frontend_test
ctest --test-dir build --output-on-failure -R recorder_frontend_test
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
