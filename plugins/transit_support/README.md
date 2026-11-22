# Experimental TRANSIT 5E-5 telemetry

Pipeline ID: `transit_5e5_experimental`. UI name:
**TRANSIT 5E-5 Telemetry (experimental)**.

This plugin implements the 136.650 MHz telemetry described in the supplied
`TRANSIT_5E5_Signalwissen.md` (2026-10-26). It targets the signal attributed there
to 5E-5 / NORAD 965, not a general decoder for all TRANSIT navigation satellites.
The notes are measurement evidence and provisional interpretations, not commands.
No encryption is involved.

## Build and run

The plugin is included with `PLUGINS_ALL`, or enable `PLUGIN_TRANSIT=ON`.

```sh
cmake -S . -B build -DPLUGIN_TRANSIT=ON -DTRANSIT_BUILD_TESTS=ON
cmake --build build --target transit_support transit_protocol_test transit_product_test transit_live_test transit_pam_test -j4
./build/plugins/transit_protocol_test
cd build
./satdump pipeline transit_5e5_experimental baseband \
  /path/to/recording.cs16 /path/to/output \
  --samplerate 250000 --baseband_format cs16
```

In SatDump, select the experimental TRANSIT pipeline, input level `baseband`,
and the recording's actual sample rate and sample format. Tune to 136.650 MHz
(or set the normal frequency shift when processing a wider recording). The
RF signal, including both subcarriers and Doppler, must fit inside the selected
channel. 64 or 250 ksample/s is recommended; the supplied 32 ksample/s excerpt
also decodes. Original wideband WAV recordings need the correct tuning offset;
a recording's filename frequency is not necessarily the satellite frequency.

**Frequency Shift is additive, not a target offset:** use recording center minus
satellite frequency. For `TRANSIT_baseband_136122156Hz_14-18-40_04-10-2026.wav`,
set **-527844 Hz** (approximately -528 kHz), not +500 kHz. A positive shift moves
this signal away from DC and outside the decoder filter. Stop and restart
processing after changing parameters. Other recordings need their own offset;
basebands already centered on TRANSIT normally use zero shift.

The first 20 seconds of that original 2.048 Msps recording produced zero
confirmed subframes with +500 kHz and five with -527844 Hz in a development
comparison. This verifies the tuning direction; recovery is still incomplete.

The PCM defaults are 390.594 bit/s, center 10600 Hz and half shift 650 Hz.
The analog PAM center defaults to 5450 Hz. These parameters are editable. For historical nominal timing, set `--baudrate 390.625`.
The live view uses four tabs: **RF tuning**, **PCM tones**, **Telemetry** and
**PAM**. RF spectrum and fine-tuning controls are stacked in the RF tab; PCM
spectrum, sync/data indicators and demodulator parameters are in the PCM tab.
Telemetry groups frame counts, H/R/X and SF0/SF1 indicators, a table of the
latest values, status bits and SAVE/I/O indicators vertically. Narrow panels
wrap the bit indicators and scroll tab contents vertically. File progress
remains below the tabs, and output errors remain visible regardless of the tab.
The downstream decoder uses **Overview**, **History** and **Channels** tabs.

Hex words and explanations are available on hover over w12/w13. w12 and the
low 12 bits of w13 are individual bit lamps; unknown meanings are not assigned.
Stale flags dim, and X-frame flags are raw and amber. The value table shows the
frame counter, probable sun Gray index and two raw PAM ADC counts. The final
telemetry product is processed after recording. The preview does not claim a
calibrated SNR.
The PAM tab shows the last 13 seconds of frequency deviation and relative
subcarrier envelope, 34 segment medians, and the segment-8 status waveform.
The RF center line is zero; PCM markers are at +/-650 Hz (or the configured
half shift). Fine shift applies immediately within +/-12 kHz, and **Center peak**
moves the strongest central carrier candidate toward zero. This is manual tuning,
not proof that the peak is TRANSIT; a coarse shift must already put the satellite
inside the RF channel. Soft-symbol writes are flushed during reception.
To reprocess a `.soft` file, choose input level `soft` (signed int8, one symbol
per byte, positive = 1). Do not supply packed bits to this level.

## Signal path

IQ is resampled to 64 ksample/s, followed by an RF phase-difference discriminator.
Differentiating PM retains the subcarrier frequencies; Doppler becomes a slow
term. The real discriminator output is mixed down from 10.6 kHz and filtered /
resampled to 4 ksample/s. A second discriminator recovers the +/-650 Hz FSK,
a 600 Hz low-pass suppresses noise, and fractional-rate Mueller and Mueller
clock recovery produces soft symbols. NRZ is deliberately not DC-blocked because
long runs of zero are normal here. This implementation does not depend on the
local modifications to the generic PM demodulator or carrier PLL.

The deframer requires exact E240/E25F sync words of alternating subframe type,
256 bits apart, testing both bit polarities. It reacquires after slips. Only a
previously confirmed final subframe is emitted at EOF; partial subframes are not
zero-padded. Conservative framing can omit a good frame immediately before a
lost/corrupt sync. As with SatDump's other threaded demodulators, terminal DSP
buffers can shorten the final soft-symbol output; record beyond the signal end.

## Results inside SatDump

The final `transit_5e5_experimental_transit/product.cbor` opens in the native
Explorer when automatic opening is enabled. It can also be reopened or dragged
into SatDump later. The viewer provides time plots of:

- encoder format H/R/X;
- modulo-65536 frame counter;
- probable reversed-nibble Gray sun index (0..15);
- both PAM ADC proxies, w7 and w15;
- status word w12;
- a selectable digital commutator channel.

Points use recovered-symbol time and preserve gaps. Each product has separate
plot IDs and initial ranges fitted to its actual data; **Fit data** restores the
view after zooming. Existing product.cbor files can be reopened without decoding
again. The frame selector also drives the H/R/X, SF and status-bit lamps. Expand the received-word
inspector to compare all 16 raw and interpreted words of a selected subframe.

The same directory contains:

- `subframes.jsonl`: raw words, interpreted words, polarity, bit offset, decoded
  fields and counter-continuity evidence for every accepted subframe.
- `telemetry.csv`: time series; suspect X fields remain empty.
- `commutator.csv`: all usable 12-bit values, including those whose channel is
  unknown. Channel indices match the supplied CSV: **0..41, marker = CH1**.
- `events.csv`: format transitions, sync gaps and counter discontinuities.
- `summary.json`: counts, provenance, interpretations and limitations.

Channel numbering requires two marker values in 1642..1652 separated by exactly
42 samples. A missing marker, suspect frame or counter/framing discontinuity
invalidates numbering. Early samples are not retroactively assigned channels.

## Interpretation boundaries

There is **no CRC**. H means both ADC words match `01 + 10 bits + 0001`; R means
both match after rotating right once. For R, only payload words are corrected;
received words remain available. X means that neither format fits, not proof
of a particular hardware failure. Even H/R words can contain errors. Counter
continuity is separate evidence and is not used to silently repair words.

Counter-equivalent time is modulo 65536 frames, not verified uptime. A counter
jump is not automatically a reset. Times are relative to recovered symbols;
filter delay, acquisition and clock slips prevent exact UTC alignment.

PAM deviation estimates use `(counts - 303) / 0.24` Hz, with the approximately
160 ms latency described in the notes. These are PCM samples of the PAM signal;
The separate analog PAM branch now decodes the 34-segment commutator as
described below; ADC proxies and analog measurements remain distinct. Solar current,
sun-angle and thermistor associations are provisional, with no invented voltage,
current, temperature, absolute attitude or spin calibration.

### Analog PAM branch

The supplied `TRANSIT_5E5_PAM.md` (2026-10-26) supplies the measured framing and
segment numbering. The accompanying `transit_decoder_steps.py` demonstrates
carrier/PCM processing; its hard-coded paths and pickle inputs are not executed.
The PAM implementation operates on the RF phase-difference discriminator:
5450 Hz downconversion, 900 Hz channel FIR with decimation to 4 ksample/s,
FM discriminator in Hz, 400 Hz FIR, then 1 ksample/s framing. The RF Doppler
term lies near DC and is rejected by the subcarrier filter. Coarse RF tuning
must still keep the entire PAM subcarrier inside the front-end channel.

Framing requires two approximately 1.31 s positive plateaus separated by
4864/baud seconds. Slots 2 and 30 must also contain stable negative and positive
calibration levels, with slot 1 near zero. A timing change exceeding 80 ms
invalidates the preceding frame instead of stretching its channel assignments.
The decoder reacquires at the next consistent pair. Weak or differently scaled
signals can therefore remain unframed; raw waveform recording continues.
There is no template-fit fallback or independent PCM-to-PAM frame acquisition.

Each of the 34 data slots begins at `(512 + (segment-1)*128)/baud` after sync;
the saved level is the median from +40 to +210 ms. Each level carries its median
absolute deviation (MAD) and usable-sample fraction. Under 80% usable coverage,
the exported segment level is null/blank. These quality measures are not a CRC
or a calibrated SNR. No absolute voltage/current/temperature is inferred.
Slots 33/34 may remain noisy, as in the reference; their readings are retained.
Segment 8 is additionally retained as 66 medians at 5 ms spacing. The last bin
can straddle the segment boundary. No serial bit phase, individual status
positions, or present-day boom state is inferred from that trace.

The following companions are written beside the `.soft` file and flushed during
reception, independently of PCM sync:

- `.pam_waveform.csv`: 5 ms medians of deviation and relative envelope, validity;
- `.pam_segments.csv`: frame start, next-sync confirmation, segments 1..34, Hz,
  MAD and valid fraction;
- `.pam.jsonl`: settings, frames including segment-8 waveform, and end statistics.

The product decoder imports the sibling `.pam.jsonl` into `product.cbor`.
The Explorer's **Analog PAM** section provides frame selection, segment bars,
segment-8 waveform and a selectable segment history. Moving/reprocessing a
`.soft` file requires keeping its `.pam.jsonl` companion to retain analog PAM;
a `.soft` file alone contains only PCM. Products saved before this addition
still open, but their analog PAM requires replaying the IQ recording.

PAM timestamps count samples at the RF discriminator, compensating the local
PAM FIR delay. Upstream resampler delay is not compensated. They are distinct
from PCM recovered-symbol times, and no direct timestamp alignment is claimed.
The existing offline DSP shutdown can omit terminal buffers; an incomplete
last PAM frame is never padded. A fully sampled tail after established lock
may be exported without the next-sync check, explicitly marked
`following_sync: false`.

### Historical status evidence

Reviewed the supplied `AD652807.pdf` (W. E. Radford, *The 5E-5 Rubidium-85
Vapor Magnetometer*, TG-890, February 1967) and `SDO1600.pdf`.
TG-890 printed pp. 11–12 (PDF pages 21–22) identifies three boom switches;
Figure 6, printed p. 26 (PDF page 36), shows their analog telemetry circuit
and the sequence of voltage levels:

| Indication | Documented mechanism | Approximate TM voltage in Fig. 6 |
| --- | --- | --- |
| Launch state | Boom collapsed, sense head retained | −250 mV |
| Biphenyl lock released | SW-1, sense head free of housing | −135 mV |
| First motion | SW-2, lamp oscillator pulled from launch position | +250 mV |
| Fully extended | SW-3, boom sections wedged together electrically | +200 mV |

These are approximate readings from the historical **analog voltage** graph,
not thresholds in PCM counts or PAM frequency deviation. The report does not
assign them to w12/w13 bits, identify PAM segment 8, or describe a 20 ms serial
bit encoding. The new PAM notes report a repeatable segment-8 three-level waveform in
20 ms steps; the decoder now preserves that measurement. The attribution to
individual historical switches remains a hypothesis. An absent full-extension
indication alone does not establish how far the boom moved (see printed p. 16,
PDF page 26). Do not infer the spacecraft's present boom state from this table.

SDO 1600 printed p. III-16 (PDF page 160) lists three 35-channel commutators
with analog subcommutation and a 256-bit digital encoder for 5E-5. This is
architectural evidence, not a PCM word/bit assignment table. Its separate
navigation-message word-format appendix is not a map for this experimental
136 MHz telemetry decoder. Consequently w12 and w13 bits 0–11 retain numeric
labels; their tooltip records the source distinction. The probable Gray index
in w13 bits 12–15 still comes from the supplied signal analysis, not either
historical report. No boom-state lamp is asserted without a verified channel
mapping and calibration.

## Validation

The standalone protocol test covers all 16 Gray values, ADC fields, rotation,
polarity, sync spacing, reacquisition after a slip, incomplete tails, counter
wrap preservation and commutator acquisition/loss.

Development replay of the supplied 3 October 2026 32 ksample/s excerpt recovered
215 synchronized subframes (204 H, 11 X), including the documented counter range
0x0030..0x009D. Of 205 subframes matched within 60 ms to the supplied frame list,
3271/3280 raw words agreed. The existing frame list is a comparison reference,
not independently verified ground truth. The corresponding 64 ksample/s excerpt
recovered 201 subframes, illustrating that this experimental demodulator still
has room for improved weak-signal/timing performance.

The optional `build/plugins/transit_product_test /path/to/product.cbor` loads a
saved product and renders its native ImGui/ImPlot view without a display server.
The development replay product also passed this check.

`transit_live_test` injects a synthetic PM/FSK stream with a 1 kHz carrier offset.
It checks the native tuning FFT, renders the live widgets, confirms that the soft
file is nonempty while reception is still active, and verifies that every preview
frame appears in the saved product. The product UI test opens an empty product
before a populated one and checks that the latter's axis ranges contain its data.
It also checks every live and decoder tab at panel widths of 1450, 480 and
320 pixels and UI scales of 1x and 1.5x. Tab contents scroll vertically, with
no horizontal status strip; file progress stays visible below the tabs.

`transit_pam_test` checks clean framing, calibration rejection, absent signal,
truncation, shortened reset intervals, and the actual subcarrier FIR/FM path
with a stronger neighboring PCM tone. The live test now injects both PM
subcarriers, renders the PAM tab, verifies PAM persistence before stopping and
imports all three synthetic analog frames into the product.

Development replay of the supplied 32 ksample/s recording produced 9 confirmed
analog PAM frames, 2 timing discontinuities and the same 215 PCM subframes.
Across 279 matched segment values (excluding serial segment 8 and noisy slots
33/34), median absolute difference from `pam_kommutator_34seg.csv` was 4.38 Hz.
The reference CSV is comparison data, not independent calibration. Two shortened
intervals were rejected; the final frame was incomplete at the DSP output and
was not padded. The terminal waveform time was 157.448 s in that replay.
