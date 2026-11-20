# TRANSIT primary documents: boom telemetry and remaining assignments

## NASA TM 80250 / NTRS 19790016560: operational NNSS timing, not 5E-5 science telemetry

Source: `/home/jankoch/Downloads/NASA_NTRS_Archive_19790016560.pdf`, *Proceedings of the Tenth Annual Precise Time and Time Interval (PTTI) Applications and Planning Meeting* (November 1978), 746 PDF pages. Full text searched for 5E-5/5B-5, 1964-083C/D, 136.65 MHz, PCM/PAM, commutator, telemetry and status assignments. The direct TRANSIT article is Theodore D. Finsod, *Transit Satellite System Timing Capabilities*, printed pp. 511–535, beginning at PDF p. 539; the proceedings also include T. Hara and K.-H. Sato's *Time Synchronization Via the Transit Satellite at Mizusawa*, already held separately as NTRS 19790016581.

Finsod's figure 2 (printed p. 514) lists the **five operational satellites as of April 1978**: 1967-34A, 1967-48A, 1967-92A, 1970-67A and 1973-81A. Neither 1964-083C nor 1964-083D is in that group. The paper analyzes the **150/400-MHz navigation message**, its 39-bit words, 6103-bit/two-minute cycle, clock-correction bits, AN/BRN-3 receiver and NAVASTROGRU ground processing. Its IBM 7094/11 appears in that **navigation** ground chain; it is not evidence about the 5E-5 136.65-MHz science decommutator or its Dictionary I/II programs. The proceedings' other PCM telemetry timing discussion concerns range/launch telemetry, not 5E-5. No 5E-5 PAM slot, PCM word, status-bit definition or flight format appears here. Do not transfer Finsod's 39-bit navigation words or clock-correction bits into the 16-bit science decoder.

Reviewed 2026-10-26. PDF page numbers are one-based; printed labels are given
separately. These findings complement `S66_DOCUMENT_FINDINGS.md` and do not
establish a mapping to PCM w12/w13 or a particular present-day PAM segment.

## TG-890 / AD652807: a concrete analog status signature

Source: `/home/jankoch/Dokumente/recv/AD652807.pdf`, W. E. Radford,
*The 5E-5 Rubidium-85 Vapor Magnetometer*, APL TG-890, February 1967.

- PDF 21, printed 11: SW-1 indicates biphenyl lock release; SW-2 indicates
  initial motion. SW-3 is the electrical contact formed when the boom elements
  wedge together at full extension. An absent indication is not a measurement
  of the exact extension length.
- PDF 36, printed 26, figure 6, visually checked: the switches select a single
  analog telemetry voltage. The plotted sequence is approximately:

| State in figure 6 | Telemetry output |
|---|---:|
| Launch | -250 mV |
| Biphenyl lock unlocked | -130 mV |
| First motion | +250 mV |
| Boom fully extended | +200 mV |

These are approximate levels read from the schematic's plotted curve, not
receiver frequency shifts, raw ADC counts, or calibrated decoder thresholds.
The drawing gives supply rails of +/-10.7 V and resistor values, but not a
present-day transfer function from voltage to recovered PAM deviation.

- PDF 26, printed 16: following the boom-extension command during the 1924 UT
  pass on 15 December 1964, the full-extension indication did not appear.
  The report interprets this as the elements apparently not making the final
  wedging contact; magnetometer/calibration-magnet data were used to investigate
  extension. This historical observation does not prove today's electrical state.

### Implication for the decoder

The documented boom indication is a multilevel analog signal. It must not be
automatically identified with the seven serial telltale positions in the S-66
reference, nor with three independent bits in w12/w13. A serial channel can
itself show three instantaneous levels (reference, half excursion, full
excursion); observing levels alone does not distinguish these encodings.

Finding the boom signal requires a 5E-5 channel assignment or independent
evidence plus voltage/deviation calibration. A constant positive PAM plateau
by itself is insufficient. Keep the existing physical assignment unconfirmed.

### Magnetometer counter: a numerical PCM identification test

TG-890 PDF 16, printed 6, visually checked, gives a gate of **0.0819265 s**
once per **0.65536 s** and the following ground conversion, with frequency
`f` in Hz and field `F` in gauss:

`F = f / (4.667393 × 10^5) − 1.34 × 10^−15 f²`.

The instrument counts **zero crossings**, so a normal sinusoidal output has
`N = 2 f × 0.0819265` crossings per gate. TG-890 PDF 27, printed 17,
independently gives 13.6 crossings as 17.8 gamma, or about **1.31 gamma per
crossing**; this checks the factor of two. Fields in the historical usable
15,000–31,000 gamma range predict about **11,472–23,710** crossing counts.
These values and smooth geographic variation can identify a candidate raw
counter field in an *early, functioning* 5E-5 PCM recording. They do not locate
its word or prove a current zero-filled register still represents valid field
measurements. The observed 2026 word groups treated as inactive science
registers must remain unlabeled pending a matching format and plausible counts.

A separate APL preprint of the later scalar-field paper is online as
[NASA NTRS 19680002123](https://ntrs.nasa.gov/api/citations/19680002123/downloads/19680002123.pdf),
PDF 4 / printed 4. It repeats the gate and conversion, describes the three
boom stages, and reports that no full-extension contact indication was
received. PDF 10 / printed 10 reports roughly 350,000 usable magnetometer
measurements received by 16 stations in TRANET, the Atlantic/Pacific Missile
Ranges and NASA STADAN. This points to historical ground records, but does not
establish that raw telemetry tapes survive. The preprint still does not identify
the PCM word or PAM commutator slot.

### Bloom 1962: instrument physics, not a 5E-5 telemetry format

Source: `/home/jankoch/Downloads/Principles of Operation of the Rubidium Vapor Magnetometer -- Bloom, Arnold L_ -- Applied Optics, #1, 1, pages 61-, 1962 jan 01 -- The Optical -- doi 10_1364_ao_1_000061 -- f681f735158e7b7ffb9da6750ddb86ef -- Anna’s Archive.pdf`,
Arnold L. Bloom, *Principles of Operation of the Rubidium Vapor Magnetometer*,
*Applied Optics* **1** (1962), pp. 61–68, DOI 10.1364/AO.1.000061.

- PDF 1–3 / printed 61–63: optical pumping gives a Larmor-frequency signal;
  the single-cell self-oscillator feeds the photocell signal back to the RF
  coil. Signal strength and correct loop phase depend on orientation. The
  single-cell example has a phase reversal across 90 degrees and a searcher
  circuit can reacquire oscillation. This is generic design theory, not a
  documented schematic of 5E-5's exact flight electronics.
- PDF 3–4 / printed 63–64: the orientation-response discussion supports
  Radford's choice of roughly 45 degrees between optical and magnetic axes.
  Bloom does not establish a PCM register, PAM slot or status flag.
- PDF 5–6 / printed 65–66: loop phase and line structure can affect frequency
  accuracy. Bloom does not give the 5E-5 zero-crossing gate or its field
  calibration; use TG-890 for those values.

Thus Bloom is useful when assessing signal loss or implausible historical
magnetometer counts, but cannot label today's w12/w13 bits.

## SDO 1600: architecture and a specific archival lead

Source: `/home/jankoch/Dokumente/recv/SDO1600.pdf`,
*Artificial Earth Satellites Designed and Fabricated by APL*, 1978.

- PDF 160, printed III-16: the 5E-5 equipment list specifies three 35-channel
  commutators with some subcommutation of analog data and a 256-bit digital
  encoder; it also lists analog three-axis solar-attitude detection.
  This does not specify present-day active commutator routing or PCM bit layouts.
- PDF 165, printed III-21, visually checked: the heliogoniometer located the
  satellite sun line to within one-tenth of a degree. The source does not tie
  this performance statement to our observed Gray-code candidate. Do not convert
  a six-bit candidate directly into a calibrated full-circle solar angle; sensor
  range, coarse/fine fields, axis selection and calibration remain unknown.
- Same page: a symposium titled *Results from the 5-E Series of Satellites*
  took place at APL/JHU on **27 September 1974**, with NASA, NOAA and APL
  participants. It reviewed scientific and engineering results, especially 5E-1.
  This is a concrete archive/library request target for programs, proceedings,
  presentation manuscripts and cited instrument reports. The source confirms
  the event, not that published proceedings exist or include an encoder map.
- PDF 164-165, printed III-20 to III-21: after early erratic operation the
  5E-5 rubidium magnetometer yielded useful data in the last two weeks of
  December 1964 and from 17 April through 8 June 1965; during the latter
  interval the entire orbit was sunlit. Prefer these historical intervals when
  testing the TG-890 zero-crossing-count hypothesis against archival frames.
  This is a statement
  about the 1964-65 experiment, not evidence that a 2026 telemetry word still
  carries valid magnetometer counts.
- PDF 165, printed III-21: reducing the ultraviolet-telescope observations
  required the vector magnetometer and heliogoniometer data together with
  Doppler-derived position. An isolated Gray-code-like PCM field is therefore
  not enough to claim a calibrated spacecraft sun line or photometer pointing.
- Appendix C, Table C-2, PDF 237 / printed C-9: entry 18 identifies
  **R. E. Fischell, J. H. Martin, W. E. Radford and W. E. Allen,
  _Radiation Damage to Orbiting Solar Cells and Transistors_, APL/JHU TG-886,
  March 1967**. A [NASA bibliography](https://ntrs.nasa.gov/api/citations/19700011921/downloads/19700011921.pdf)
  supplies the additional accession **AD 657 155**. The full report appears
  [search-indexed at CiteSeerX](https://citeseerx.ist.psu.edu/document?doi=b0e543b4e2f44a31c7fb412d6d30740c9a28caef&repid=rep1&type=pdf),
  but its PDF could not be retrieved in this review. The same authors' 1965
  Bergen proceedings chapter with this title is already held and its 5E-5
  experiment tables were analyzed below. TG-886 may contain added details,
  but is a lower-priority acquisition than a 5E-5 channel/word assignment.
- TG-890 PDF 49, initial external distribution list, identifies Navy contract
  **NOw 62-0604-c**, the Bureau of Naval Weapons and Special Projects Office
  as provenance for that instrument report. Use the contract with 5E-5 and
  report numbers in archival searches; the contract alone is broad and also
  covers unrelated APL work. TG-890 PDF 48 contains a correction notice for
  printed pp. 33 and 36, with replacement leaves appended as PDF pp. 50-51;
  no change to the boom-telemetry figure on printed p. 26 is indicated.

## Ausgabe: pass timing, not onboard attitude

Source: `/home/jankoch/Dokumente/recv/Ausgabe.pdf`, one page, visually checked.
Header: OPS 6582 (TRANSIT 5B-5), 64083D, orbit 5844. It tabulates a pass on
2026-10-04 from 14:11:11 to 14:28:47.

- The largest listed elevation is 78.27 degrees at 14:20:27; the Dop. column
  is 17 there, between 765 at 14:19:31 and -735 at 14:21:22.
- This can help cross-check recording time and Doppler progression after
  verifying time zone and Doppler reference frequency/units, which are not
  printed. Do not silently assume UTC or local time.
- The numeric `Ausleuchtung` column is not explained in this PDF. Do not treat
  it as a calibrated solar angle or infer an eclipse transition from it.
- Ground-station azimuth/elevation do not establish spacecraft body attitude
  or the meaning of an onboard status bit.

No production decoder labels, thresholds or bit assignments changed.

## Ness 1970: 5E-5 magnetometer acquisition cadence

Source: `/home/jankoch/Downloads/19700015897.pdf`, Norman F. Ness,
*Magnetometers for Space Research*, NASA-TM-X-63887 / X-690-70-78, March 1970.
`/home/jankoch/Downloads/magnetometers-for-space-research-5dvydi0k20.pdf`
is another 187-page scan of the same report; its bytes/OCR differ. The relevant
printed pages 100-101 were visually checked in the NASA-numbered scan
(PDF pages 103-104).
`/home/jankoch/Downloads/BF00183028.pdf` is the **96-page journal version**
in *Space Science Reviews* **11** (1970), pp. 459–554, not a new spacecraft
telemetry manual. Section 5.2.5 is at PDF 67–68 / printed 525–526. It repeats
the 0.65-s cadence and does not supply a PCM bit map, PAM channel list or
status-word legend.

- Section 5.2.5 explicitly identifies 1964-83C as the Rb-85 scalar magnetometer
  satellite. A counter measured its resonance frequency **every 0.65 s** with
  **0.01% precision or better**. The counter result was transmitted as a
  **digital number** and converted to field magnitude on the ground.
- The text quotes an observed magnetic-field range of about 15,000-31,000 gamma
  and recounts no positive full-extension boom indication. Historical measured
  ranges are not a digital count-to-field transfer function or a 2026 value.
- The preceding account on printed page 99 concerns **1963-38C / 5E-1** and
  must not be transferred to 5E-5.
- This makes a candidate 5E-5 PCM field testable for one new count per ~0.655-s
  subframe. Ness alone contains no specific 16-bit word index, bit order,
  counter gate interval, scaling or calibration equation; **TG-890 and NTRS
  19680002123 do give the gate and conversion**, as noted above.
- Its reference list (PDF 151, printed 148) cites A. J. Zmuda, W. E. Radford,
  F. T. Heuring and P. Verzariu, *The Scalar Magnetic Intensity at 1100
  Kilometers in Middle and Low Latitudes*, JGR **73**, 2495-2503 (1968), as
  follow-up science. This is an optional calibration/context lead, not a
  guaranteed channel map.

## Two geodesy compilations: identity only

- `/home/jankoch/Downloads/19780003602.pdf`: NASA SP-365 Part I, final report
  of the National Geodetic Satellite Program (1977). PDF page 66 / printed 61,
  table 1.18, visually checked, lists **5E-5 = 1964-83C** and **Oscar-02 =
  1964-83D** with Doppler frequency code Y for 5E-5 (162/324 MHz). It does not
  identify the 136.65-MHz experimental telemetry subcarriers or PCM fields.
- `/home/jankoch/Downloads/19690002139.pdf`: Joseph E. Gross III,
  *Preprocessing Electronic Satellite Observations*, NASA CR-1183 (1968).
  PDF page 142 / printed 129, table 8, visually checked, repeats the same
  satellite identities and Doppler frequency coding. The report's focus is
  preprocessing geodetic observations, not 5E-5 experiment decoding.

These identity tables are relevant to the recurring 83C/83D ambiguity. The
reported Y-frequency pair is a **Doppler** listing and must not be mistaken for
a statement that the 136.65-MHz experiment transmission never existed.

## NASA CP-3153 (1992): historical magnetic survey context

Source: `/home/jankoch/Downloads/19920019377.pdf`, *Types and Characteristics
of Data for Geomagnetic Field Modeling*, NASA Conference Publication 3153,
edited by R. A. Langel and R. T. Baldwin, 1992. Relevant article: *Satellite
Data for Geomagnetic Field Modeling* by Langel and Baldwin.

- PDF 102 / printed 92: explicitly says lack of an onboard recording device
  limited the historical coverage of 1964-83C to reception near ground
  stations. It also notes spacecraft magnetic field interference above the
  instrument noise. This reinforces that the historical magnetic survey used
  real-time reception, but does not prove anything about today's data validity.
- PDF 120 / printed 110, table 6, visually checked: 1964-83C is listed with
  rubidium magnetometer, approximately 22 nT accuracy, 1040-1089 km altitude,
  historical observation span December 1964 to June 1965, coverage near ground
  stations. The 22 nT is an approximate accuracy for historical processed data,
  not a PCM count scale or a present-day accuracy claim.
- This is a geomagnetic-data compilation. It does not supply PCM bit positions,
  a digital counter conversion equation, a PAM schedule, or status-flag meanings.

## Fischell et al.: 1964-83C radiation experiment inventory

Source: `/home/jankoch/Downloads/Radiation Trapped in the Earth’s Magnetic Field_ Proceedings of the Advanced Study Institute Held at the Chr. Michelsen Institute, Bergen, Norway August 16–September 3, 1965.pdf`, R. E. Fischell, J. H. Martin, W. E. Radford and W. E. Allen, *Radiation Damage to Orbiting Solar Cells and Transistors*, printed pp. 793–807 (PDF pp. 796–810). Tables I, IV and V were visually checked.

- Table I (printed p. 794 / PDF 797) explicitly includes **1964-83C**, the object identified elsewhere as 5E-5. It lists solar-cell circuit B and points to Tables IV and V for the transistor experiments. Its quoted ±1% is a historical telemetry-system accuracy, not a calibration for individual decoded fields.
- Table V (printed p. 797 / PDF 800) is explicitly titled **1964 83C degradation experiments**. Experiments 1–9 measure transistor beta (gain), 10–18 transistor leakage, 19 capacitor leakage, and 20–23 capacitor short-circuit behaviour. The transistor types are 2N2222, 2N2586 and 2N2907-A; the table distinguishes unshielded, 0.1-inch aluminium and 0.3-inch aluminium devices. These are *experiment numbers*, not proven PAM slots or PCM channel numbers.
- Its footnote says the solar-cell experiments were **similar** to those of 1963-38C. Table IV (printed p. 796 / PDF 799) lists that earlier spacecraft's eight solar-cell tests with different cover materials/thicknesses. Similarity makes these useful hypotheses, but does not establish the exact 5E-5 solar-cell wiring or channel order. Table IV's 2N1711 transistor experiments 9–10 belong to **1963-38C**, not 5E-5.
- The text on printed p. 798 / PDF 801 states that several experiments did not yield meaningful data and describes corrections to historical solar-cell readings. The following plots present selected 1964-83C component degradation, so they help identify expected slow trends. A flat or failed channel in a present-day recording cannot be assigned solely from this chapter.
- No telemetry-word layout, PAM commutator position, status-word bit legend, or per-experiment channel address appears in the chapter. Use the inventory to compare independently identified channels and seek an APL telemetry specification, experiment interface document, or ground-station channel list for the actual mapping.

## NASA 19660004280 / APL S1P-86-65: predecessor 5E-1, not 5E-5

Source: `/home/jankoch/Downloads/19660004280.pdf`, APL, *Satellite 1963 38C*, S1P-86-65, October 1965. Cover and printed pp. 4–5 (PDF 1, 4–5) were visually checked; the whole text was searched for bit/channel assignments.

- The title and introduction identify **1963-38C = 5E-1**. This is a predecessor analogy, not a 1964-83C/5E-5 telemetry specification.
- Printed p. 4 gives the predecessor's digital encoder: one 16-bit sync word, one 16-bit frame counter, fourteen 16-bit data registers, and 256-bit subframes. Twelve registers are time-shared across two subframes; one other register contains **eight telltale bits** and the digitized analog commutator reading. The report does not list the eight telltale meanings or their bit positions.
- If 5E-5 retained this register pattern, an eight-bit telltale field alongside an ADC value is a testable hypothesis for one current PCM word. It is an inference from 5E-1 and must be checked against 5E-5 data; it does not identify w12 or w13.
- Printed p. 5 says **two 35-channel commutators**, one normally digitized into PCM at one channel per subframe, the other normally read as analog PAM at twice that rate; routing can be switched by command and the analog commutator can be stopped on one channel. The text says “35”, despite imperfect OCR reading it as “33” in places. This is a useful architectural model for testing correlations but not proof of the 5E-5 routing or PAM slot map. SDO 1600 explicitly lists **three** 35-channel commutators for 5E-5, a material difference.
- Printed pp. 2–3 (PDF 2–3) report a 136.650-MHz telemetry carrier, ~10.3-kHz digital subcarrier at 195.3 bit/s, and ~5.4-kHz PAM subcarrier. The bit rate differs from the current 5E-5 experimental decoder's ~390.6 bit/s. Shared RF architecture must not be used to copy its detailed timing or register meanings.
- The introduction says 1963-38C charged-particle and auxiliary data had been transferred to NASA's Space Sciences Data Center at Goddard, and the report refers to “Dictionary II” tapes. An analogous 5E-5 data dictionary, tape description, or APL satellite report is a more specific archival target than this predecessor report.

## Historical rate and restored-data lead for the next decoder pass

- A. M. Smith, *Stellar Photometry from a Satellite Vehicle* (`/home/jankoch/Downloads/19660014433.pdf`), printed p. 4 / PDF 5, explicitly describes 1964-83C photometer data as sent every 1.31 s at **195.3 bit/s**, with **15-bit** capacity for each of two photometers. This is a historical 5E-5 measurement, not merely a 5E-1 analogy. It differs by a factor of two from the current ~390.594-bit/s recording and from the current decoder's allowed 380–400-bit/s parameter range. The date or mechanism of the rate change is not established by Smith. Do not claim that the 390-bit/s mode was unchanged since launch; support both rates as separate, validated decoding modes when historical recordings are processed.
- The user-supplied NSSDCA catalog `/home/jankoch/Downloads/transit5e5_mag_64-083c-01a_DSC_0032.pdf` states that magnetometer dataset **64-083C-01A** was restored from a 7-track BCD tape to an ASCII tape, with **DR004385 / DS004385 / D000242**, time span 17 December 1964–30 June 1965. It says the old tape dataset was migrated to archival information packages. Request the restored data *and its record layout, codebook, processing notes and associated documents*. This is processed scalar-field data, not proof that raw PCM tapes survive; it can nonetheless validate magnetometer time/value physics and reveal references to the processing chain.
- The 5E-1 report above identifies “Dictionary II” tapes for NASA Space Sciences Data Center transfer. Ask NSSDCA/APL whether a corresponding 5E-5 dictionary, PCM frame schedule, PAM/commutator allocation, telltale truth table, 15-bit photometer interface, or ground decommutation program survives. Search both 1964-83C and 1964-83D because object designations have been inconsistent in later catalogs; keep physical spacecraft identity separate from a catalog row.
- A closer reading of the 5E-1 report, printed pp. 7–8, names the *actual science-data processing chain*: Pulse Data Processor (bit identification and analog digitization), IBM 1401 pass check, then an **IBM 7094 Dictionary I program that decommutated the digital message**. Dictionary I preserved time-correlated raw data; Dictionary II converted counts to physical quantities. This is a much better software-provenance lead than the AN/UYK-1 submarine navigation computer. It is still a 5E-1 description, so request the corresponding **5E-5** program, input/output format and channel tables rather than assuming the same mapping.
- The 5E-5 NSSDCA catalog's scanned PDF p. 7 includes a 1967 APL-supplied description of **D000242**: 1333 records of 80 characters, with UT, year/day, latitude, east longitude, radial distance and geomagnetic scalar intensity; pp. 8–13 are sample *processed magnetometer* listings. The catalog says Favin and Zmuda supplied it and that the original was created on an **IBM 7094**. This confirms processing provenance and a useful archive identifier, but the restored AIP is not itself the full raw PCM/PAM format or a status-bit legend.

## NASA 19690009548: unrelated PCM encoder design

Source: `/home/jankoch/Downloads/19690009548.pdf`, *Final Report for PCM Encoder System*, Space and Tactical Systems Corporation for NASA Manned Spacecraft Center, contract NAS9-8360, 31 January 1969. The cover was visually checked and the full extracted text searched for 5E-5, 1964-83C/D, Transit and APL; none occur. Its specification describes flexible **8-bit** words, 16–64 words per frame and an analog multiplexer expandable to 35 inputs. The shared terms “PCM”, “35 channels” and “subcommutator” do not make this a 5E-5 encoder document: the sponsor, date and stated frame format differ from the APL 16-bit, 256-bit telemetry observed here. Do not transfer its sync, word layout, ADC format or channel assignments into the decoder. This is a low-priority generic architecture reference only.

## NASA TM X-63121: 5E-1 electron measurements, not a 5E-5 channel map

Source: `/home/jankoch/Downloads/19680008713.pdf`, D. J. Williams, J. F. Arens and L. J. Lanzerotti, *Observations of Trapped Electrons at Low and High Altitudes*. The instrumentation passage on printed pp. 4–5 (PDF pp. 8–9) and references were visually checked.

- The paper compares Explorer 26 with **1963-38C = Transit 5E-1**, rather than 1964-83C/5E-5. It names two 1000-micron surface-barrier solid-state detectors on 5E-1 measuring integral electron intensities above about **280 keV** and **1.2 MeV**. The spacecraft was magnetically aligned; the detectors viewed perpendicular to its alignment axis.
- Its figures use those electron intensities to compare radiation-belt behavior during 1965 magnetic storms. These energy thresholds explain a specific scientific product of the predecessor, but they do not assign any 5E-5 PCM word, PAM slot or status bit, and cannot be copied into the 5E-5 decoder as channel labels.
- The cited next source for detailed 5E-1 instrument construction is D. J. Williams and A. M. Smith, *Daytime Trapped Electron Intensities at High Latitudes at 1100 kilometers*, J. Geophys. Res. 70, 541–556 (1965). Another cited paper, C. O. Bostrom, J. W. Kohl and D. J. Williams, *The February 5, 1965, Solar Proton Event, 1, Time History and Spectrum Observed at 1100 km*, J. Geophys. Res. 72, 4487–4495 (1967), may further characterize the 5E-1 proton spectrometer. Seek a 5E-5-specific payload/channel schedule before applying either to 5E-5.

## Wyatt 1981: APL organizational lead, no 5E telemetry specification

Source: `/home/jankoch/Downloads/02-01-Wyatt.pdf`, Theodore Wyatt, *The Gestation of Transit as Perceived by One Participant*, Johns Hopkins APL Technical Digest 2(1), 32–38 (1981). All seven pages were text-searched and the program-organization passage checked.

- The 1964 APL Space Department organization on printed p. 33 explicitly includes a **Telemetry and Command Group** under the Data and Control Branch, alongside an Operations Control Group and a Reports Office. Those names are useful archive-request terms for original engineering reports, ground decommutation material and command/telltale documentation.
- The article is a program history. It does not name 5E-5, 1964-83C or a 5E-series PCM/PAM channel map. Its illustrations show generic early Transit concepts and a 5C-1 cutaway; neither should be treated as a 5E-5 wiring diagram. No status-bit label or decoder constant can be derived from it.

## TRW-130 / AN-UYK-1: navigation processor, not the 5E-5 science bit map

Source: `/home/jankoch/Downloads/M250-2U19_AN_UYK-1_RefMan_Feb63.pdf`, TRW, *AN/UYK-1 Machine Reference Manual*, M250-2U19 Rev. February 1963. The scanned file has no text layer; cover, contents and printed pp. 1-2 and 1-3 (PDF pp. 1–5, 10–11) were visually checked. Context: Lauren J. Rueger, *Development of Receivers to Characterize Transit Time and Frequency Signals*, Johns Hopkins APL Technical Digest 19(1), 53–59 (1998), especially printed pp. 56–57.

- The manual describes the **TRW-130/AN-UYK-1 general-purpose processor**: 8192 basic core words, expandable to 32768; each word contains **15 data bits plus a parity bit**; its architecture and I/O instructions are the manual's subject. That format is a CPU storage word, not evidence about a 5E-5 16-bit PCM word. The manual's table of contents lists machine organization, commands and programming aids, not a Transit application program or telemetry decommutation table.
- Rueger places Ramo Wooldridge's computer/data processor in the **AN/BRN-3 submarine navigation system**, handling the operational 150/400-MHz Transit navigation message, orbital parameters and Doppler measurements. The 156 × 39-bit navigation message every two minutes must not be conflated with 5E-5's 136.65-MHz experimental telemetry.
- There is a narrower ground-station lead: Rueger says modified AN/BRN-3 receivers at Winter Harbor, Rosemont, Point Mugu and Oahu had widened IF bandwidth to accommodate other telemeter signals. This establishes possible receiver hardware reuse, **not** that its AN/UYK-1 software decoded 5E-5 science channels.
- For the computer route, seek the **AN/BRN-3 application program/listing, data-processor interface description, and ground-station modifications**, but prioritize an APL 5E-5 telemetry decommutator program, channel dictionary, or processed-data codebook for status and PAM assignments. A machine reference manual alone cannot supply those meanings.

## Guier and Weiffenbach 1997/1998: orbit computation history, not telemetry software

Source: `/home/jankoch/Downloads/19-01-Guier.pdf`, W. H. Guier and G. C. Weiffenbach, *Genesis of Satellite Navigation*, Johns Hopkins APL Technical Digest 19(1), 14–17 (1998), reprinted from 18(2), 178–181 (1997). All four pages were read.

- The described 1957–58 computing work digitized **Sputnik Doppler measurements** and estimated orbit parameters; APL used a **Univac 1200F** for the early orbit-inference study. This predates both the AN/UYK-1 Transit shipboard navigator and 5E-5.
- The article clearly distinguishes the future tracking station's orbit determination, the injection station's uplink of orbit parameters, and the submarine receiver/computer's position solution. Those are navigation functions and cannot be used as the 5E-5 scientific telemetry decommutator or its status-bit map.
- The author note identifies Guier as supervisor of APL's Space Analysis and Computation Group in 1964. That group name is an archive-search lead for **tracking/orbit software**, whereas the Telemetry and Command Group from Wyatt is the better lead for 5E-5 PCM/PAM processing.

## APL TG 1305 / DTIC ADA037398: navigation-system status, not 5E-5 science telemetry

Source: `/home/jankoch/Downloads/DTIC_ADA037398.pdf`, H. D. Black, R. E. Jenkins and L. L. Pryor, *The Transit System, 1975*, APL/JHU TG 1305, December 1976. The cover and report-documentation page identify it as **unclassified** and approved for unlimited public distribution. Its 61 PDF pages were text-searched for 5E-5, 1964-083C/D, 136.65 MHz, telemetry and commutator; no 5E-5-specific format appears. The report is about the operational navigation constellation, Doppler tracking, ephemerides and the 150/400-MHz navigation broadcast. Its section 16 on a future flight computer and extra message concerns a 25-bit/s additional *navigation* message, not the 5E-5 PCM/PAM channel map. Do not transfer this message rate or computer functions to the experimental decoder.

## NASA NTRS 19790016581: Transit time transfer, not science telemetry

Source: `/home/jankoch/Downloads/NASA_NTRS_Archive_19790016581.pdf`, T. Hara and K.-H. Sato, *Time Synchronization via the Transit Satellite at Mizusawa*. All 11 pages were text-searched. The authors use a TRANET I / NACODE receiver, Doppler tracking and a satellite timing pulse to compare the Mizusawa and USNO clocks, chiefly using 1976 observations; the paper's example satellite is identified as 30190. It does not describe 5E-5, its 136.65-MHz scientific link, 256-bit subframes, PAM channels, or status bits. The discussion of receiver delay and time marks belongs to navigation time transfer and yields no decoder constant for the 5E-5 science stream.

## IDA P-2852 / DTIC ADA286476: operational navigation overview

Source: `/home/jankoch/Downloads/DTIC_ADA286476.pdf`, H. M. Federhen,
*Worldwide Radionavigation Systems*, Institute for Defense Analyses, IDA
Paper P-2852, September 1993. Chapter IV (printed pp. 63-81, PDF pp. 76-94)
describes the operational Transit navigation constellation and its user/ground
segments. Its Table 8 (printed p. 66 / PDF p. 79) specifies 150/400-MHz
navigation carriers and a two-minute ephemeris/time-mark message. Printed
pp. 67-68 describe 6103 bits arranged as 156 39-bit words and a final 19-bit
mark. The extracted text was searched for 5E-5, 1964-83C, 136.65 MHz,
commutators, PAM/PCM, photometers and telemetry-status terms; none yielded a
5E-5 format. This document cannot supply the 136.65-MHz science decoder's
256-bit PCM schedule, analog PAM assignments or status-bit meanings.
