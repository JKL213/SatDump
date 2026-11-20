# S-66 reference and Space Materials Handbook: findings

Reviewed 2026-10-26 against the user-supplied PDFs. Page numbers below distinguish
one-based PDF pages from printed page labels. These are reference findings;
they do not establish TRANSIT 5E-5 PCM word/bit assignments.

## Sources

- `/home/jankoch/Downloads/19640015076_Redacted.pdf`: Polar Ionosphere Beacon
  Satellite (S-66), Operations Plan 2-63; NASA X-533-63-29 / TM-X-55009, 1963.
  98 PDF pages. Relevant tables and figures are readable despite the filename.
- `/home/jankoch/Downloads/19700011921.pdf`: Space Materials Handbook,
  Third Edition, NASA SP-3051, Rittenhouse and Singletary, 1969. 737 PDF pages.
  This is NOT the TG-886 experimental report itself.

## Documented S-66 telltale format

Primary anchors:

- PDF 21, printed 13, section 4.3.7.1: seven ON/OFF functions, synchronized
  with main PAM channel 9. Main channels have 75% duty cycle and nominal
  duration 0.629 s. Do not transplant this duration into TRANSIT's measured timing.
- PDF 22, figure 5: the telltale register has seven inputs, synchronized through
  a trigger module and feeding the PAM commutator.
- PDF 24-25, printed 14-15, table 4-1: complete main and subcommutator assignments.
- PDF 62, figure 10b: illustrated numbered pulse positions 1-7, position 8
  omitted for synchronization, followed by another readout. In this illustration
  information pulses point DOWN from the reference level. Absolute voltage sign
  or instantaneous frequency sign alone must not be used as the logical value.
- PDF 65, printed 51, section 6.2.4.2.3.1: seven information bits; 0 is full
  pulse height, 1 is half height. Nominal pulse width approximately 25 ms with an
  equally long interval between pulses. Timing varies with temperature and
  voltage; the eighth position is omitted for synchronization.

S-66 subcommutator #2 (PCM within PAM channel 9), documented slot order:

| Position | Source label | Interpretation in this analysis |
|---|---|---|
| 1 | Oven Current | Oscillator-oven current indication |
| 2 | Charge Regulator | Charge regulator indication |
| 3 | 20, 41/41 and 360 mc Transmitters | Transmitter group; apparent frequency typo in original table |
| 4 | 162,324 mc Transmitters | Second transmitter group |
| 5 | Solar Boost | Solar boost indication; do not invent active-high semantics |
| 6 | Active Temperature Control | Active temperature-control indication |
| 7 | Electron Density Probe | Electron-density-probe indication |

The source's command diagram (figure 7, PDF 27) uses 20/40/41/360 for the first
transmitter group. Preserve the table's original wording when transcribing it.
The list identifies functions, not an exhaustive truth table of physical states.
For example, naming the charge-regulator input does not by itself say which
logical state means normal regulation versus override.

### Separation and panel deployment are separate in S-66

Table 4-1 places separation telemetry / magnetometer regulator voltage on main
channel 8 and blade erection / +21.4 V converter voltage on main channel 28.
They are NOT among the seven listed telltale inputs. Thus the conjectural list
of separation, despin, panels, boom and magnet states in secondary discussions
must not be treated as a documented seven-bit assignment for TRANSIT.

### Two different uses of PCM

The S-66 term "PCM" here describes the serial telltale subcommutator within an
analog PAM channel. It is not an assignment of TRANSIT's separate 256-bit FSK
frame or its words w12/w13. S-66 section 4.3.7.1 instead describes both subcarriers
normally carrying the PAM signal, with electron-density data substituted on the
10.5 kHz subcarrier every eighth frame. Similar subcarrier frequencies do not
establish equivalent payload formats.

### Implications for a TRANSIT test

1. Fit a seven-data-position/eighth-omission model to raw PAM segment 8, retaining
   the measured TRANSIT timing rather than fixing the S-66 25 ms value.
2. Fit reference level, half/full excursion and both discriminator polarities.
   A return/reference level is not a third logical state.
3. Identify slot 1 from the omitted eighth position, not solely from the PAM
   channel boundary; distinguish register synchronization from PAM dead time.
4. Check repetition, pulse width, equal inter-pulse spacing, and missing-slot
   behavior across frames and recordings. Do not force a seven-bit interpretation
   onto data that lack the expected sync interval or repeat structure.
5. If supported, label values only as T1-T7 (experimental) until an independent
   5E-5 source proves the physical names. There is still no evidenced connection
   to PCM w12/w13.

No TRANSIT physical labels or production decoding were changed by this review.

## Materials handbook: useful experiment inventory, not a telemetry map

- PDF 417, printed 413: text identifies transistor reliability experiments on
  satellite 1964-83C and points to table 14-4 and reference 14.
- PDF 418, printed 414, table 14-4, visually checked including footnotes:
  1964-83C carried the listed 2N2222 and 2N2586 experiments with unshielded and
  0.1-inch aluminium-shielded variants, and a shielded 2N2907-A experiment.
  Values are retained fractions of original transistor beta after specified
  orbital durations, not present-day telemetry calibration factors.
  The separate 2N1711 intact/punctured-case rows belong to 1963-38C, not 5E-5.
- PDF 434, printed 430, section 14.3 reference 14: R. E. Fischell et al.,
  Radiation Damage to Orbiting Solar Cells and Transistors, TG-886,
  AD 657 155, N67-38181, APL/JHU, March 1967.

TG-886 remains a useful acquisition target for experiment wiring, measured
quantities and further references. The handbook does not identify which
present-day digital commutator channel or PCM bit belongs to each experiment.
