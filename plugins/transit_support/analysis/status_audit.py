#!/usr/bin/env python3
"""Reproducible, read-only audit of the four historical TRANSIT frame exports.

Input words in the Oct 4 exports are ALREADY rotated. Never rotate them again.
Rotation is applied only to raw mf_frames.txt words to verify provenance.
No physical flag names or production decoder settings are changed.
"""
import argparse
import base64
import csv
import hashlib
import html
import json
import platform
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection
import markdown
import numpy as np

PERIOD = 256 / 390.594
TIME_TOL = 0.025  # exports rounded to 10 ms; allow observed clock variation
PASSES = {
    "p1337": ("03.10.2026 · 13:37 MESZ", "transit_frames_2026-10-03_1137utc.txt", 159.9075),
    "p1238": ("04.10.2026 · 12:38 MESZ", "2026-10-04/p1238_frames.txt", 197.8725),
    "p1418": ("04.10.2026 · 14:18 MESZ", "2026-10-04/p1418_frames.txt", 613.065),
    "p1601": ("04.10.2026 · 16:01 MESZ", "2026-10-04/p1601_frames.txt", 555.4425),
}


def gray(v):
    result = v
    while v:
        v >>= 1
        result ^= v
    return result


def decode_field(word, low, width, encoding):
    value = (word >> low) & ((1 << width) - 1)
    if encoding == "reversed_gray":
        value = int(f"{value:0{width}b}"[::-1], 2)
    return gray(value) if encoding != "binary" else value


def circular_delta(a, b, width):
    half = 1 << (width - 1)
    return (b - a + half) % (2 * half) - half


def ror16(value, shift):
    shift %= 16
    return ((value >> shift) | (value << ((16 - shift) % 16))) & 65535


def adjacent(a, b):
    return (a["structural"] and b["structural"] and a["rot"] == b["rot"]
            and a["sf"] != b["sf"] and abs(b["t"] - a["t"] - PERIOD) < TIME_TOL)


def counter_ok(a, b):
    return b["words"][8] == (a["words"][8] + int(b["sf"] == 0)) % 65536


def read_frames(path, pid, raw_path):
    raw = []
    for line in raw_path.read_text().splitlines()[1:]:
        parts = line.split()
        raw.append((float(parts[3]), int(parts[1]), [int(x, 16) for x in parts[5:21]]))
    rows = []
    for index, line in enumerate(path.read_text().splitlines()[1:]):
        parts = line.split()
        if not parts:
            continue
        start = 2 if pid == "p1337" else 4
        words = [int(x, 16) for x in parts[start:start + 16]]
        if len(words) != 16:
            raise ValueError(f"Malformed frame: {path}:{index + 2}")
        sf, t = int(parts[1]), float(parts[0])
        rot = 0 if start == 2 else int(parts[2])
        adc = all((words[i] & 0xc00f) == 0x4001 for i in (6, 14))
        sync = words[0] == (0xe240 if sf == 0 else 0xe25f)
        # Same rounded timestamp and SF; raw file can contain competing candidates.
        candidates = [w for rt, rs, w in raw if abs(rt - t) < .011 and rs == sf]
        corrected = [[w[0]] + [ror16(v, rot) for v in w[1:]] for w in candidates]
        matches = sum(w == words for w in corrected)
        closest = min(corrected, key=lambda w: sum((a ^ b).bit_count() for a, b in zip(w, words))) if corrected else None
        differences = [] if closest is None else [f"w{i+1:02d}:{a:04X}->{b:04X}"
                                                 for i, (a, b) in enumerate(zip(closest, words)) if a != b]
        rows.append(dict(pid=pid, row=index + 2, t=t, sf=sf, rot=rot, words=words,
                         adc=adc, sync=sync, structural=adc and sync, strict=False,
                         raw_candidates=len(candidates), raw_matches=matches,
                         raw_differences=";".join(differences) if closest is not None else "no timestamp candidate",
                         gray6_raw_match=closest is not None and (closest[12] & 0xfc00) == (words[12] & 0xfc00)))
    if any(b["t"] <= a["t"] for a, b in zip(rows, rows[1:])):
        raise ValueError(f"Non-monotonic/duplicate frame times in {path}")
    for a, b in zip(rows, rows[1:]):
        if adjacent(a, b) and a["rot"] == 0 and counter_ok(a, b):
            a["strict"] = b["strict"] = True
    return rows


def write_csv(path, rows, fields=None):
    if fields is None:
        fields = list(rows[0])
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def candidate_scores(data):
    scores = []
    fields = [(12, low, width) for low, width in [(0, 4), (4, 4), (8, 4), (12, 4), (0, 8), (8, 8), (0, 16)]]
    fields += [(13, 16 - n, n) for n in range(4, 9)]
    for pid, rows in data.items():
        for quality in ("structural_H", "counter_H", "counter_H_raw_verified"):
            pairs = [(a, b) for a, b in zip(rows, rows[1:]) if adjacent(a, b) and a["rot"] == 0
                     and (quality == "structural_H" or counter_ok(a, b))
                     and (quality != "counter_H_raw_verified" or (a["raw_matches"] > 0 and b["raw_matches"] > 0))]
            for word, low, width in fields:
                for encoding in ("binary", "gray", "reversed_gray"):
                    ds = [circular_delta(decode_field(a["words"][word - 1], low, width, encoding),
                                         decode_field(b["words"][word - 1], low, width, encoding), width)
                          for a, b in pairs]
                    scores.append(dict(pass_id=pid, quality=quality, word=word, low_bit=low, width=width,
                                       encoding=encoding, edges=len(ds), changed=sum(d != 0 for d in ds),
                                       unit_steps=sum(abs(d) == 1 for d in ds), large_steps=sum(abs(d) > 1 for d in ds),
                                       positive=sum(d > 0 for d in ds), negative=sum(d < 0 for d in ds)))
    return scores


def bit_audit(data):
    stats, transitions = [], []
    for pid, rows in data.items():
        for rotation in sorted({r["rot"] for r in rows}):
            usable = [r for r in rows if r["structural"] and r["rot"] == rotation]
            pairs = [(a, b) for a, b in zip(rows, rows[1:]) if adjacent(a, b) and a["rot"] == rotation]
            for word in (12, 13):
                for bit in range(16):
                    val = lambda r: (r["words"][word - 1] >> bit) & 1
                    changes = [(a, b) for a, b in pairs if val(a) != val(b)]
                    single = sum(adjacent(a, b) and adjacent(b, c) and a["rot"] == rotation
                                 and val(a) == val(c) != val(b) for a, b, c in zip(rows, rows[1:], rows[2:]))
                    stats.append(dict(pass_id=pid, rotation=rotation, word=word, bit=bit, frames=len(usable),
                                      ones=sum(val(r) for r in usable), adjacent_edges=len(pairs),
                                      transitions=len(changes), counter_checked_transitions=sum(counter_ok(a, b) for a, b in changes),
                                      isolated_one_frame_excursions=single))
                    for a, b in changes:
                        transitions.append(dict(pass_id=pid, rotation=rotation, word=word, bit=bit,
                                                previous_frame_s=a["t"], current_frame_s=b["t"],
                                                old=val(a), new=val(b), counter_ok=counter_ok(a, b),
                                                source_row=b["row"]))
    return stats, transitions


def plot_bits(pid, rows, out):
    title, _, duration = PASSES[pid]
    fig, axes = plt.subplots(3, 1, figsize=(15, 9), sharex=True,
                             gridspec_kw={"height_ratios": [1, 5, 5]})
    qnames = ["Format unsicher", "Rotation ≠ 0", "H / Struktur", "H / Zähler"]
    qcolors = ["#9b9b9b", "#d68a22", "#2d75a8", "#16765b"]
    for r in rows:
        q = 3 if r["strict"] else 2 if r["structural"] and r["rot"] == 0 else 1 if r["structural"] else 0
        axes[0].plot([r["t"], r["t"] + PERIOD], [q, q], lw=3, color=qcolors[q], solid_capstyle="butt")
    axes[0].set_yticks(range(4), qnames, fontsize=8)
    for ax, word in zip(axes[1:], (12, 13)):
        polygons, colors = [], []
        for i, r in enumerate(rows):
            # Never bridge missing data or allow a cell to overlap the next sample.
            end = min(r["t"] + PERIOD, rows[i + 1]["t"] if i + 1 < len(rows) else duration)
            for bit in range(16):
                polygons.append([(r["t"], bit - .44), (end, bit - .44), (end, bit + .44), (r["t"], bit + .44)])
                one = (r["words"][word - 1] >> bit) & 1
                colors.append(("#145a75" if one else "#dce6ec") if r["structural"] and r["rot"] == 0
                              else ("#bd8446" if one else "#f0e4d6"))
        ax.add_collection(PolyCollection(polygons, facecolors=colors, edgecolors="none"))
        ax.set_ylim(-.6, 15.6)
        ax.set_yticks(range(16), [f"b{b}" for b in range(16)], fontsize=8)
        ax.set_ylabel(f"w{word}")
        ax.grid(axis="x", alpha=.15)
    axes[-1].set_xlim(0, duration)
    axes[-1].set_xlabel("Sekunden seit Aufnahmestart · Weiß = keine exportierten Frames")
    fig.suptitle(f"{title} — w12 / w13 als Einzelbits\nDunkel = 1, hell = 0 · Blau = H mit Formatprüfung · Braun = übrige Exportwerte", fontsize=12)
    fig.tight_layout(rect=[0, 0, 1, .94])
    path = out / f"{pid}_bits.png"
    fig.savefig(path, dpi=140)
    plt.close(fig)
    return path


def plot_gray(data, out):
    fig, axes = plt.subplots(4, 2, figsize=(15, 12), sharex="row")
    for row, (pid, records) in enumerate(data.items()):
        usable = [r for r in records if r["structural"] and r["rot"] == 0]
        for col, width in enumerate((4, 6)):
            ax = axes[row, col]
            get = lambda r: decode_field(r["words"][12], 16 - width, width, "reversed_gray")
            ax.scatter([r["t"] for r in usable], [get(r) for r in usable], color="#c17e25", s=9, label="Struktur H")
            strict = [r for r in usable if r["strict"]]
            ax.scatter([r["t"] for r in strict], [get(r) for r in strict], color="#12677b", s=12, label="zusätzlich Zählernachbar")
            for a, b in zip(records, records[1:]):
                if adjacent(a, b) and a["rot"] == 0 and abs(get(a) - get(b)) <= 1:
                    ax.plot([a["t"], b["t"]], [get(a), get(b)], color="#12677b", lw=.6)
            ax.set_title(f"{PASSES[pid][0]} · Bits 15…{16 - width}", fontsize=10)
            ax.set_ylim(-1, (1 << width))
            ax.set_xlim(0, PASSES[pid][2])
            ax.set_ylabel(f"Gray-Index modulo {1 << width}")
            ax.grid(alpha=.2)
            if row == 0:
                ax.legend(fontsize=8)
            if row == 3:
                ax.set_xlabel("Sekunden seit Aufnahmestart")
    fig.suptitle("w13: bisherige 4 Bit und Kandidat mit 6 Bit\nBitreihenfolge umgekehrt, dann Gray → Binär · keine Verbindung über Lücken / große Sprünge", fontsize=13)
    fig.tight_layout(rect=[0, 0, 1, .94])
    path = out / "gray_candidates.png"
    fig.savefig(path, dpi=140)
    plt.close(fig)
    return path


def evidence_runs(data):
    """Export all observed runs of the upper six bits, retaining glitches and gaps."""
    result = []
    for pid, records in data.items():
        last = None
        for r in records:
            if not (r["structural"] and r["rot"] == 0):
                last = None
                continue
            code = r["words"][12] & 0xfc00
            if last is not None and adjacent(last, r) and code == (last["words"][12] & 0xfc00):
                result[-1]["end_frame_s"] = r["t"]
                result[-1]["frames"] += 1
                result[-1]["counter_supported_frames"] += int(r["strict"])
            else:
                result.append(dict(pass_id=pid, start_frame_s=r["t"], end_frame_s=r["t"], frames=1,
                                   w13_mask_fc00=f"{code:04X}", gray6=decode_field(code, 10, 6, "reversed_gray"),
                                   counter_supported_frames=int(r["strict"])))
            last = r
    return result


def checks():
    # Meaningful guards: bit order, cyclic boundary, export normalization, gaps and wrap.
    assert decode_field(0x0400, 10, 6, "reversed_gray") == 63
    assert decode_field(0x0000, 10, 6, "reversed_gray") == 0
    assert decode_field(0x1400, 10, 6, "reversed_gray") == 48
    assert decode_field(0x1c00, 10, 6, "reversed_gray") == 47
    assert circular_delta(63, 0, 6) == 1
    assert ror16(0xb136, 1) == 0x589b
    a = dict(t=0, sf=1, rot=0, structural=True, words=[0] * 16)
    b = dict(t=PERIOD, sf=0, rot=0, structural=True, words=[0] * 16)
    a["words"][8] = 65535
    assert adjacent(a, b) and counter_ok(a, b)
    assert not adjacent(a, dict(b, t=2 * PERIOD))
    assert not adjacent(a, dict(b, rot=1))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--pam-prefix", type=Path, help="Optional SatDump PAM companion prefix for p1337 (without .pam.jsonl)")
    args = parser.parse_args()
    checks()
    out = args.output_dir
    out.mkdir(parents=True, exist_ok=True)
    data, sources, quality = {}, [], []
    for pid, (_, relative, _) in PASSES.items():
        source = args.data_root / "transit_decoded" / relative
        raw = args.data_root / f"prep_{pid}" / "mf_frames.txt"
        sources.extend([source, raw])
        rows = data[pid] = read_frames(source, pid, raw)
        if len({r["t"] for r in rows}) != len(rows):
            raise ValueError("Duplicate timestamps")
        for rot in sorted({r["rot"] for r in rows}):
            subset = [r for r in rows if r["rot"] == rot]
            edges = [(a, b) for a, b in zip(rows, rows[1:]) if adjacent(a, b) and a["rot"] == rot]
            quality.append(dict(pass_id=pid, rotation=rot, exported_frames=len(subset),
                                structural_frames=sum(r["structural"] for r in subset),
                                counter_supported_H_frames=sum(r["strict"] for r in subset),
                                adjacent_edges=len(edges), counter_ok_edges=sum(counter_ok(a, b) for a, b in edges),
                                raw_verified_frames=sum(r["raw_matches"] > 0 for r in subset)))
    normalized = []
    for pid, rows in data.items():
        for r in rows:
            normalized.append(dict(pass_id=pid, source_row=r["row"], time_s=r["t"], sf=r["sf"],
                                   export_rotation=r["rot"], adc_format=r["adc"], exact_sync=r["sync"],
                                   counter_supported_H=r["strict"], raw_matches=r["raw_matches"],
                                   raw_differences=r["raw_differences"], gray6_raw_match=r["gray6_raw_match"],
                                   **{f"w{i + 1:02d}_hex": f"{w:04X}" for i, w in enumerate(r["words"])},
                                   gray4=decode_field(r["words"][12], 12, 4, "reversed_gray"),
                                   gray6_candidate=decode_field(r["words"][12], 10, 6, "reversed_gray")))
    scores = candidate_scores(data)
    stats, transitions = bit_audit(data)
    for name, records in [("normalized_frames", normalized), ("quality", quality), ("field_candidates", scores),
                          ("bit_statistics", stats), ("bit_transitions", transitions), ("gray6_runs", evidence_runs(data))]:
        write_csv(out / f"{name}.csv", records)
    hypotheses = [
        dict(field="w13 b15..b10", level="beobachtet", finding="Laengere Gray-Schrittfolgen nach Bitumkehr", evidence="gray6_runs.csv; field_candidates.csv", limitation="Ausreisser, Luecken; keine CRC"),
        dict(field="w13 b10/b11", level="wahrscheinlich", finding="Fortsetzung desselben Gray-Felds statt unabhaengiger Schaltflags", evidence="p1337 82.36s; p1418 233.21s; p1601 388.74s", limitation="Feldbreite und physikalische Bedeutung nicht dokumentiert"),
        dict(field="w13 Gray-Feld", level="offen", finding="Sensorname, Winkelmassstab, Nullrichtung", evidence="Keine unabhaengige Kalibrierung", limitation="Auch mehr als sechs Bit nicht ausgeschlossen"),
        dict(field="w13 b9..b0", level="offen", finding="Keine physikalische Zuordnung", evidence="bit_statistics.csv; bit_transitions.csv", limitation="b9 koennte ebenfalls zum Gray-Feld gehoeren"),
        dict(field="w12 b15..b0", level="offen", finding="Kein ueber alle Durchgaenge ueberzeugendes einfaches Feldmodell", evidence="field_candidates.csv", limitation="Nur feste Nibble-/Byte-/16-Bit-Felder getestet, keine vollstaendige Suche"),
        dict(field="R-Abschnitte", level="beobachtet", finding="ADC-Format oft passend, bisheriges Zaehler-Modell nicht", evidence="quality.csv", limitation="Bedeutungszuordnung aus diesen Abschnitten ausgesetzt"),
        dict(field="physikalische Einzelbitnamen", level="nicht belegt", finding="Keine dokumentierte Belegung in dieser Auswertung", evidence="Quellendiskussion in report.md", limitation="Historische Vorschlaege nicht als gesicherte Namen uebernehmen"),
    ]
    write_csv(out / "hypotheses.csv", hypotheses)
    images = [plot_gray(data, out)] + [plot_bits(pid, rows, out) for pid, rows in data.items()]
    pam_report = ""
    if args.pam_prefix:
        from pam_link_audit import audit_pam
        pam_report, pam_plot, pam_sources = audit_pam(data["p1337"], args.pam_prefix, out)
        images.append(pam_plot)
        sources.extend(pam_sources)
    manifest = dict(python=platform.python_version(), numpy=np.__version__, matplotlib=matplotlib.__version__, markdown=markdown.__version__,
                    nominal_subframe_seconds=PERIOD, adjacency_tolerance_seconds=TIME_TOL,
                    source_files=[dict(path=str(p), bytes=p.stat().st_size, sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in sources],
                    script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                    total_exported_frames=len(normalized), raw_verified_frames=sum(r["raw_matches"] > 0 for r in normalized),
                    gray6_raw_verified_frames=sum(r["gray6_raw_match"] for r in normalized),
                    internal_checks="passed")
    if args.pam_prefix:
        import scipy
        manifest["scipy"] = scipy.__version__
        manifest["pam_analysis_script_sha256"] = hashlib.sha256(Path(__file__).with_name("pam_link_audit.py").read_bytes()).hexdigest()
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    make_report(out, data, quality, scores, images, manifest, pam_report)
    print(json.dumps(dict(output=str(out.resolve()), **{k: manifest[k] for k in ("total_exported_frames", "raw_verified_frames", "internal_checks")}), indent=2))


def make_report(out, data, quality, scores, images, manifest, pam_report):
    qtable = "| Durchgang | Rotation | Frames | Struktur | Nachbarpaare | Zähler passend | Rohvergleich |\n|---|---:|---:|---:|---:|---:|---:|\n"
    for q in quality:
        qtable += f"| {q['pass_id']} | {q['rotation']} | {q['exported_frames']} | {q['structural_frames']} | {q['adjacent_edges']} | {q['counter_ok_edges']} | {q['raw_verified_frames']} |\n"
    stable_table = "| Durchgang | Filter | Paare | ±1-Schritte | Größere Sprünge |\n|---|---|---:|---:|---:|\n"
    for s in scores:
        if s["word"] == 13 and s["width"] == 6 and s["encoding"] == "reversed_gray":
            stable_table += f"| {s['pass_id']} | {s['quality']} | {s['edges']} | {s['unit_steps']} | {s['large_steps']} |\n"
    text = f"""# TRANSIT 5E-5: Prüfung von w12 / w13

## Ergebnis

**w13 b10 und b11 sind gute Kandidaten für die Fortsetzung des bisherigen Gray-Felds. Sie sollten vorerst nicht als unabhängige Schaltflags bezeichnet werden.** Die Bits 15…10 ergeben nach Bitumkehr und Gray-Dekodierung längere Schrittfolgen. Das ist ein empirischer Befund, keine dokumentierte Sensorzuordnung. Die tatsächliche Feldbreite, physikalische Skala, Nullrichtung und Polarität bleiben offen. Auch sieben Bit sind anhand dieser Daten nicht ausgeschlossen. Ein glatter Verlauf allein entscheidet die Feldbreite nicht.

* 03.10., p1337: 54 → 55 → … → 63 → 0 → 1 → … → 7. Der Übergang von 0x0400 zu 0x0000 (jeweils mit 0xFC00 maskiert) bei **82,36 s** betrifft b10. Daneben bleiben Ausreißer sichtbar: Index 48 bei 81,05 s, Rücksprung 7 → 0 bei 142,42 s und Index 42 bei 143,08 s. Diese Werte wurden nicht stillschweigend repariert.
* 04.10., p1418: im längeren H-Abschnitt 55 → 54 → … → 36. Bei **233,21 s** wird maskiert 0x1400 zu 0x1C00 (48 → 47), also b11 gesetzt.
* 04.10., p1601: im H-Abschnitt 55 → 54 → … → 43. Derselbe b11-Übergang erscheint bei **388,74 s**. Das wiederkehrende Codeverhalten stützt die Feldhypothese; die Zählerqualität ist hier schwächer. Das unmittelbare Übergangspaar besteht dort die Zählerprüfung nicht, wohl aber Paare innerhalb der Plateaus davor und danach.
* p1238 ist lückenhaft und enthält Sprünge. Dieser Durchgang bestätigt keine sichere Zuordnung.

Zeitangaben sind Zeitstempel des ersten exportierten Frames mit dem neuen Wert, keine millisekundengenauen Sensor-Schaltzeiten. Die genannten Folgen fassen wiederholte Werte zusammen und können Lücken enthalten; sie sind keine lückenlose Aufzeichnung.

**w12 bleibt unzugeordnet.** Nibble-, Byte- und 16-Bit-Felder wurden als Binär-, Gray- und bitumgekehrter Gray-Code geprüft. Keine dieser einfachen Interpretationen erklärt die wechselnden Werte über alle Durchgänge überzeugend. Langsame oder konstante Bits dürfen daraus weder Schalter- noch Sensornamen bekommen. Bei w13 bleiben insbesondere b9…b0 unzugeordnet.

## Qualität und Normalisierung

{manifest['total_exported_frames']} exportierte Frames wurden eingelesen; {manifest['raw_verified_frames']} stimmen vollständig mit einem zeitlich passenden Rohkandidaten aus mf_frames.txt überein, nachdem ausschließlich der Rohkandidat um die deklarierte Rotation korrigiert wurde. Der Rohvergleich bestätigt die Verarbeitung, nicht die Fehlerfreiheit des empfangenen Signals.

Die fünf abweichenden Frames liegen in p1337 bei 4,77 / 18,29 / 39,92 / 84,98 / 138,49 s. Die Unterschiede sind in normalized_frames.csv wortweise aufgeführt. Bei 138,49 s betrifft die Abweichung w13 b8 (Rohwort A003, Export A103); die anderen Unterschiede liegen außerhalb w12/w13. Die hier untersuchten sechs oberen w13-Bits stimmen in **allen {manifest['gray6_raw_verified_frames']} Frames** mit dem Rohkandidaten überein. Ein zusätzlicher Filter „counter_H_raw_verified“ verwirft abweichende Frames ganz und ist im Feldvergleich enthalten.

**Die Wörter der Exportdateien sind bereits rotiert.** Die Analyse dreht sie nicht nochmals. Die Tabelle führt alle Rotationen getrennt auf.

{qtable}

„Struktur“ bedeutet exaktes, zum SF passendes Syncwort sowie gültige Formatbits in w7 und w15. Nachbarpaare sind aufeinanderfolgende Originalzeilen mit gleichem Rotationsmodus, wechselndem SF und Abstand 256/390,594 s ±25 ms. Es wird nicht über übersprungene oder ungültige Zeilen verbunden. Die Zählerprüfung erwartet +1 beim Übergang zu SF0, ansonsten denselben Wert, jeweils modulo 65536. Ein streng markierter H-Frame grenzt an mindestens ein solches geprüftes Paar. Dies ist keine CRC-Prüfung und garantiert nicht die Integrität von w12/w13.

In den R-Abschnitten bestehen die ADC-Formatbits häufig die Prüfung, die Zählerfolge jedoch nicht. Diese Daten bleiben in den Bitbildern und CSVs enthalten, werden aber nicht als Grundlage für physikalische Zuordnungen verwendet. Ob der Zähler dort anders kodiert ist oder eine zusätzliche Dekodierabweichung vorliegt, ist offen.

## Sensitivität des 6-Bit-Kandidaten

{stable_table}

Gezählt sind zyklische Differenzen modulo 64. Unveränderte Werte zählen zu „Paare“, nicht zu „±1“. Große Sprünge bleiben enthalten. Ein 4-Bit-Ausschnitt kann ebenfalls glatt sein, aber Teilabschnitte einer längeren Folge zurückfalten; allein der Anteil kleiner Sprünge beweist sechs Bit daher nicht. Alle getesteten Breiten 4…8 und Kodierungen stehen in field_candidates.csv. Es gibt keinen nachträglich erfundenen Signifikanzwert und keinen unabhängigen Blindtest: Die Auswertung ist explorativ, mit Wiederholung über unterschiedliche Durchgänge.

## Was die historischen Quellen tatsächlich belegen

Die ältere Untersuchung [1964-83D, 2010](https://emitters.space/1964-83D.pdf) diskutiert im PAM-Telltale-Segment mögliche Zustände von Auslegern, Solarpaneelen und weiteren Einrichtungen. Sie liefert keine belastbare Bitpositionstabelle für diese PCM-Wörter. Die spätere [PCM-Untersuchung](https://emitters.space/Satellite%201964-83D%20PCM%20Telemetry%20Decoded.pdf) enthält ebenfalls keine vollständige physikalische Zuordnung der hier untersuchten Einzelbits. Vorschläge für andere Satelliten sind keine Belegung von TRANSIT 5E-5.

Die mitgelieferten Signalbilder und Notizen sind hilfreiche Hypothesenquellen. Ihre Beschriftung „Status/Flags“ wird hier nicht als Nachweis behandelt. Insbesondere bedeutet „erfolgreich demoduliert“ nicht automatisch „Bedeutung bekannt“.

## Nächste entscheidende Messung

1. w13 Bits 15…10 neben dem bisherigen Gray-Index als **Kandidat** beobachten; b9 separat mitführen, damit die tatsächliche Breite prüfbar bleibt. Kein Winkel in Grad ohne unabhängige Kalibrierung.
2. PCM und PAM bis auf die einzelnen Pulsflanken synchronisieren; eine PCM-Bitänderung muss wiederholt zur gleichen PAM-Pulsposition passen. Der ergänzende Vergleich unten, sofern mit PAM-Dateien ausgeführt, prüft bereits den gemeinsamen analogen Signalinhalt. Die Zuordnung von Pulspositionen zu PCM-Einzelbits bleibt offen.
3. Für w12 zunächst einzelne, über mehrere Frames anhaltende Änderungen mit PAM-Pulspositionen, Betriebsmodus und Kommutatorphase vergleichen. Gemeinsam mit der Zeit driftende Sensoren liefern sonst Scheinkorrelationen. Wiederholte Gegenbeispiele ausdrücklich behalten.
4. Einen physikalischen Namen erst mit einem passenden Kanalplan/Schaltplan oder mehreren voneinander unabhängigen Beobachtungen vergeben. Statisch bleibende Bits lassen sich aus diesen kurzen Überflügen allein nicht identifizieren.

## Dateien und Reproduktion

* normalized_frames.csv: sämtliche Wörter, Originalzeile, Rotationsangabe, Qualitätsmerkmale und beide Gray-Indizes.
* quality.csv: Qualitätsübersicht; bit_statistics.csv: Häufigkeiten, Wechsel und isolierte Ein-Frame-Ausschläge je Bit/Rotation.
* bit_transitions.csv: alle Wechsel zwischen zulässigen Nachbarpaaren; keine Überbrückung von Lücken.
* gray6_runs.csv: beobachtete Folgenstücke inklusive Ausreißern; field_candidates.csv: vollständiger Vergleich der getesteten Feldmodelle.
* hypotheses.csv: Trennung zwischen Beobachtung, wahrscheinlicher Interpretation und offener Zuordnung, jeweils mit Beleg und Grenze.
* manifest.json: Eingabepfade, SHA-256, Versionen und Prüfparameter. Keine Eingabedatei wurde verändert.

Der Analyzer steht unter plugins/transit_support/analysis/status_audit.py. Die Ausgabe ändert weder die Live-Anzeige noch die Produktionsdekodierung. Bitnummern: b0 ist das niederwertigste Bit; Wortnummern sind 1-basiert.

## Abbildungen

"""
    if pam_report:
        text = text.replace("## Dateien und Reproduktion", pam_report + "## Dateien und Reproduktion")
    text += "\n\n".join(f"![{p.stem}]({p.name})" for p in images) + "\n"
    (out / "report.md").write_text(text)
    # Standalone review artifact: all figures embedded; report also readable offline.
    options = "".join(f'<option value="{i}">{html.escape(p.stem)}</option>' for i, p in enumerate(images))
    figures = "".join(f'<img id="plot{i}" alt="{html.escape(p.stem)}" style="display:{"block" if i == 0 else "none"}" src="data:image/png;base64,{base64.b64encode(p.read_bytes()).decode()}">' for i, p in enumerate(images))
    document = f'''<!doctype html><html lang="de"><meta charset="utf-8"><title>TRANSIT Statuswort-Analyse</title>
<style>body{{font:16px/1.55 system-ui;margin:2rem auto;max-width:1500px;padding:0 1rem;color:#192d37;background:#f7f9fa}}img{{width:100%;background:white}}article{{background:white;padding:1.5rem;border:1px solid #ccd5da;max-width:1100px;margin:auto}}select{{font:inherit;padding:.5rem}}h1{{font-size:1.7rem}}h2{{font-size:1.3rem;margin-top:2rem}}a{{color:#12677b}}table{{border-collapse:collapse;font-size:.9rem;width:100%}}td,th{{padding:.4rem;border-bottom:1px solid #ccd5da;text-align:left}}th{{background:#e8f0f3}}li{{margin:.6rem 0}}</style>
<h1>TRANSIT 5E-5 · Statuswörter</h1><p><b>Neuer Kandidat:</b> w13 b10/b11 gehören vermutlich zum Gray-Feld. Für w12 gibt es noch keine belastbare Zuordnung.</p>
<label for="figure">Abbildung: </label><select id="figure">{options}</select>{figures}
<p><a href="report.md">Bericht als Markdown</a> · <a href="normalized_frames.csv">Frames</a> · <a href="bit_transitions.csv">Bitwechsel</a> · <a href="field_candidates.csv">Feldvergleich</a> · <a href="manifest.json">Quellen/Prüfparameter</a></p>
<details open><summary>Vollständiger Bericht und Qualitätsgrenzen</summary><article>{markdown.markdown(text.split('## Abbildungen')[0], extensions=['tables'])}</article></details>
<script>document.getElementById('figure').addEventListener('change',function(){{document.querySelectorAll('img').forEach((im,i)=>im.style.display=i===Number(this.value)?'block':'none')}});</script></html>'''
    (out / "report.html").write_text(document)


if __name__ == "__main__":
    main()
