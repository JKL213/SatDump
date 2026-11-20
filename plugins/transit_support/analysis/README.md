# Historical status-word audit

Current synthesis: [research status](TRANSIT_5E5_FORSCHUNGSSTAND_2026-10-27.md),
[historical source index](TRANSIT_5E5_QUELLENINDEX_2026-10-27.md),
[speculative channel work map](TRANSIT_5E5_KANALRATEN.md), and
[cross-cutting guess model](TRANSIT_5E5_GUESSMODELL.md). The source index states
which original PDFs remain locally available and which conclusions rely on
earlier page-checked notes.

`status_audit.py` reads the four 3–4 October 2026 frame exports and the corresponding
`prep_p*/mf_frames.txt` files. It does not modify recordings or decoder settings.
The narrative findings refer specifically to these captures, not arbitrary inputs.

Dependencies: Python 3.10+, NumPy, Matplotlib, Python-Markdown; SciPy for the optional
PAM comparison. On the analysis host:

```sh
MPLCONFIGDIR=/tmp/transit-mpl /usr/bin/python3 \
  plugins/transit_support/analysis/status_audit.py \
  --data-root /home/jankoch/Dokumente/recv \
  --output-dir /home/jankoch/SatDump/build/transit_status_audit \
  --pam-prefix /home/jankoch/SatDump/build/transit_pam_validation/transit_5e5_experimental
```

Open `report.html` for the self-contained report and selectable figures. The PNGs,
Markdown, CSVs and source-hash manifest are also written separately. The input
exports are already rotated; only raw comparison words are rotated to check their
provenance. Object/pickle NumPy files are never loaded.

Checks cover reversed Gray ordering, the cyclic boundary, raw-word rotation,
counter rollover and rejection of time gaps / mode changes. All original rows,
including suspect frames and one-frame outliers, remain in the normalized export.
The analysis distinguishes format-compatible H data, counter-supported H data,
and the stricter subset with fully matching raw words. There is no CRC guarantee.

The finding that w13 b10/b11 may extend the Gray field is **exploratory**. The
absolute sensor identity, field width and physical scale remain unconfirmed.
The report does not establish physical names for w12 or the remaining w13 bits.

The optional PAM comparison uses the already generated `.pam_waveform.csv` and
`.pam.jsonl` from the **same p1337 recording**. It fits a shared time offset and
ADC-to-frequency line on the first 80 seconds, evaluates them on the later portion,
and exports the 66 raw samples of each accepted PAM status segment. It does not
claim that an ADC plateau correlation identifies the timing of individual 20 ms
pulses. Omit `--pam-prefix` to analyze only the four historical PCM exports.
