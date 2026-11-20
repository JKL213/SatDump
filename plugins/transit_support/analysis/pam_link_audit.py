"""Optional PCM/PAM cross-check for the 3 October recording, not flag naming."""
import csv
import json
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import spearmanr


def save_csv(path, rows):
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def audit_pam(rows, prefix, out):
    sources = [Path(str(prefix) + ".pam_waveform.csv"), Path(str(prefix) + ".pam.jsonl")]
    wave = np.genfromtxt(sources[0], delimiter=",", names=True)
    wt, wy = wave["sample_time_s"], wave["deviation_hz"]
    if len(wt) < 2 or not np.all(np.diff(wt) > 0):
        raise ValueError("PAM waveform must have strictly increasing times")
    times, counts, identities = [], [], []
    omitted = 0
    for r in rows:
        if not r["strict"]:
            continue
        for word in (7, 15):
            adc = (r["words"][word - 1] >> 4) & 1023
            # Near-full-scale samples of the noisy last slots distort the analog fit.
            if adc >= 800:
                omitted += 1
                continue
            times.append(r["t"] + (word - 1) * 16 / 390.594)
            counts.append(adc)
            identities.append((r["row"], word))
    times, counts = np.array(times), np.array(counts)
    train = times < 80
    test = ~train
    if min(sum(train), sum(test)) < 20:
        raise ValueError("Too few PCM samples for the fixed two-part PAM comparison")
    offsets = np.arange(-.65, .6501, .005)

    def sample_at(offset):
        query = times + offset
        # Never interpolate invalid samples, time gaps or beyond the recording.
        right = np.searchsorted(wt, query)
        inside = (right > 0) & (right < len(wt))
        right = np.clip(right, 1, len(wt) - 1)
        good = inside & (wt[right] - wt[right - 1] < .008)
        good &= (wave["valid"][right] != 0) & (wave["valid"][right - 1] != 0)
        values = np.interp(query, wt, wy)
        return values, good & np.isfinite(values)

    trials = []
    for offset in offsets:
        values, valid = sample_at(offset)
        select = train & valid
        trials.append(dict(offset_s=float(offset), train_pairs=int(sum(select)),
                           train_spearman=float(spearmanr(values[select], counts[select]).statistic)))
    best = max(trials, key=lambda row: row["train_spearman"])
    values, valid = sample_at(best["offset_s"])
    train_valid, test_valid = train & valid, test & valid
    slope, intercept = np.polyfit(values[train_valid], counts[train_valid], 1)
    test_rho = float(spearmanr(values[test_valid], counts[test_valid]).statistic)
    test_mae = float(np.median(np.abs(counts[test_valid] - (intercept + slope * values[test_valid]))))
    near = [r["offset_s"] for r in trials if r["train_spearman"] >= best["train_spearman"] - .005]
    pairs = [dict(source_row=identities[i][0], word=identities[i][1], nominal_word_time_s=float(t),
                  pam_time_s=float(t + best["offset_s"]), adc_count=int(counts[i]),
                  pam_deviation_hz=float(values[i]), valid_pair=bool(valid[i]), split="first_80s" if train[i] else "remaining")
             for i, t in enumerate(times)]
    save_csv(out / "pam_alignment_scores.csv", trials)
    save_csv(out / "pam_adc_pairs.csv", pairs)
    frames = [json.loads(line) for line in sources[1].read_text().splitlines()]
    frames = [f for f in frames if f.get("kind") == "frame"]
    if not frames or any(len(f["status8_hz_5ms"]) != 66 for f in frames):
        raise ValueError("Expected validated PAM frames with 66 status-segment samples")
    status = np.array([f["status8_hz_5ms"] for f in frames], dtype=float)
    samples = [dict(frame_start_s=f["start_s"], sample_bin=i, offset_in_segment_s=i * .005,
                    deviation_hz=v if v is not None else "", interpretation="unassigned")
               for f in frames for i, v in enumerate(f["status8_hz_5ms"])]
    save_csv(out / "pam8_samples.csv", samples)

    fig, axes = plt.subplots(3, 1, figsize=(12, 11), gridspec_kw={"height_ratios": [1, 1.2, 1.3]})
    ax = axes[0]
    ax.plot(offsets, [r["train_spearman"] for r in trials], color="#12677b")
    ax.axvline(best["offset_s"], color="#c17e25", ls="--", label="Maximum im ersten Zeitabschnitt")
    ax.set(xlabel="PAM-Zeit minus nomineller PCM-Wortzeit (s)", ylabel="Spearman ρ", ylim=(-1, 1))
    ax.legend(fontsize=9)
    ax.grid(alpha=.2)
    ax = axes[1]
    for mask, label, color in [(train_valid, "erste 80 s (Anpassung)", "#12677b"),
                                (test_valid, "Rest (gleicher Versatz / gleiche Gerade)", "#c17e25")]:
        ax.scatter(values[mask], counts[mask], s=12, alpha=.65, label=label, color=color)
    x = np.linspace(-450, 450, 100)
    ax.plot(x, intercept + slope * x, color="#333333", lw=1)
    ax.set(xlabel="Analoger PAM-Hub (Hz)", ylabel="w7 / w15 ADC-Zählerstand")
    ax.legend(fontsize=9)
    ax.grid(alpha=.2)
    ax = axes[2]
    im = ax.imshow(np.ma.masked_invalid(status), aspect="auto", origin="lower", cmap="coolwarm", vmin=-450, vmax=450,
                   extent=[0, 330, -.5, len(frames) - .5], interpolation="nearest")
    ax.set_yticks(range(len(frames)), [f"{f['start_s']:.2f} s" for f in frames], fontsize=9)
    ax.set(xlabel="Zeit im PAM-Segment 8 (ms), 5-ms-Raster", ylabel="Rahmenstart", title="PAM 8: gemessene Pegel, keine zugeordneten Statusbits")
    fig.colorbar(im, ax=ax, label="Hub (Hz)", fraction=.025)
    fig.suptitle("03.10.2026 · PAM und PCM: Abgleich und Statussegment", fontsize=14)
    fig.tight_layout(rect=[0, 0, 1, .96])
    plot = out / "pam_pcm_link.png"
    fig.savefig(plot, dpi=140)
    plt.close(fig)
    result = dict(train_pairs=int(sum(train_valid)), test_pairs=int(sum(test_valid)),
                  omitted_adc_ge_800=omitted, offset_s=best["offset_s"], train_rho=best["train_spearman"],
                  test_rho=test_rho, slope=float(slope), intercept=float(intercept),
                  test_median_absolute_error_counts=test_mae, near_optimal_offset_min_s=min(near),
                  near_optimal_offset_max_s=max(near), pam_frames=len(frames), source_files=[str(p) for p in sources])
    (out / "pam_link_summary.json").write_text(json.dumps(result, indent=2) + "\n")
    report = f"""## Ergänzung: PAM / PCM und Segment 8

Für den 3. Oktober wurden zusätzlich die bereits erzeugten PAM-Dateien desselben Mitschnitts eingelesen. Aus zählergestützten H-Frames wurden w7/w15 entnommen; {omitted} Werte ≥800 wurden als Bereich außerhalb des normalen analogen Vergleichs ausgelassen. Als nominelle Zeit dient der jeweilige Wortanfang (Framezeit + 96 bzw. 224 Bitzeiten). Ein gemeinsamer Versatz wurde nur anhand der ersten 80 s gesucht: −650…+650 ms in 5-ms-Schritten. Ungültige PAM-Samples, Lücken und Extrapolation werden verworfen.

* Anpassung: {result['train_pairs']} Paare, Spearman ρ = **{result['train_rho']:.4f}**; gewählter Versatz **{result['offset_s']*1000:.0f} ms**.
* Späterer Zeitabschnitt mit unverändertem Versatz: {result['test_pairs']} Paare, ρ = **{test_rho:.4f}**. Das ist eine zeitliche Gegenprüfung im selben Überflug, kein unabhängiger Überflug.
* Gerade nur aus dem ersten Abschnitt: ADC ≈ {intercept:.2f} + {slope:.4f} × Hub[Hz]. Im späteren Abschnitt beträgt der mediane absolute Fehler {test_mae:.2f} Zählerstände. Dies bestätigt empirisch den gemeinsamen Signalinhalt, ist keine offizielle Kalibrierung.
* Versätze mit ρ höchstens 0,005 unter dem Maximum liegen zwischen **{min(near)*1000:.0f} und {max(near)*1000:.0f} ms**. Das ist ein empirischer Empfindlichkeitsbereich, kein Konfidenzintervall; die volle Kurve steht in pam_alignment_scores.csv. Wegen der Segmentplateaus ist das Maximum nicht zeitlich eindeutig. Der Versatz fasst Messzeitpunkt, Zeitbasis und Demodulatorverzögerung zusammen.

Die **{len(frames)} akzeptierten PAM-Rahmen** liefern außerdem je 66 Rohwerte im 5-ms-Raster für Segment 8 (pam8_samples.csv). Das Bild zeigt die wiederkehrenden positiven, negativen und mittleren Pegel, einschließlich Übergängen und Rauschen. Es gibt noch keine Zuordnung einer Pulsposition zu einem PCM-Bit oder Schalter. Für etwa 20-ms-Pulse muss die Zeitlage anhand der Flanken genauer bestimmt werden; das breite Korrelationsmaximum allein genügt dafür nicht. Die drei nicht übernommenen vollständigen Rahmen der älteren 12-Rahmen-Auswertung sind hier nicht ergänzt worden.

"""
    return report, plot, sources
