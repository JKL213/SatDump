# HANDOFF — Übergabeprotokoll

Neuester Eintrag oben. Format je Eintrag: Datum, Werkzeug, Getan, Geänderte Dateien,
Verifiziert, Offen, Annahmen.

## 2026-11-23 — Codex

**Getan**
- Transit-Live-Oberfläche auf vier Tabs verteilt: RF tuning (Spektrum und Feinabstimmung), PCM tones (Spektrum und Demodulationswerte), Telemetry (Format, Werte, Statusbits und Ausgabe), PAM (Analogdiagnosen).
- Lange horizontale Statuszeile in den Decoder-Modulen entfernt. Werte stehen in Tabellen/vertikalen Abschnitten, Statusbits umbrechen bei Platzmangel. Tabinhalte scrollen vertikal; Dateifortschritt/Abort bleiben darunter. Schreibfehler erscheinen unabhängig vom aktiven Tab.
- Nachgeschalteten Telemetrie-Decoder in Overview, History und Channels gegliedert; Kanalwerte als Tabelle dargestellt.
- FFT-Achsenbeschriftung rechts an der tatsächlichen Textbreite ausgerichtet. Bestehende kompakte Produktinspektor-Ansicht bleibt erhalten.
- Bestehende UI-Tests auf Tabwechsel, schmale Panels, Skalierung und fehlenden horizontalen Überlauf umgestellt; README aktualisiert.

**Geänderte Dateien**
- `plugins/transit_support/module_transit_demod.cpp`
- `plugins/transit_support/module_transit_decoder.cpp`
- `plugins/transit_support/telemetry_widgets.h`
- `plugins/transit_support/tuning_spectrum.h`
- `plugins/transit_support/tests/product_test.cpp`
- `plugins/transit_support/tests/live_test.cpp`
- `plugins/transit_support/README.md`
- `HANDOFF.md`

**Verifiziert (per Befehl geprüft)**
- `cmake --build build -j4 --target transit_support transit_product_test transit_live_test satdump-ui` erfolgreich; geänderte Transit-Ziele anschließend erneut gebaut.
- Aus `build/`: `./plugins/transit_product_test transit_pam_validation/transit_5e5_experimental_transit/product.cbor` erfolgreich. Alle Tabs bei 1450/480/320 px und 1x/1.5x UI-/Schriftskalierung geprüft, inklusive eingebettetem Live-/Dateimodus und sichtbarem Fortschrittsbereich.
- Aus `build/`: `./plugins/transit_live_test` erfolgreich. Gefüllte Live-Tabs bei 1100/480/320 px und 1x/1.5x ohne horizontalen Überlauf; 59 Subframes, 15360 Soft-Bytes und 3 PAM-Frames gespeichert; vorhandene EOF-/Importprüfungen bestanden.

**Offen**
- Keine interaktive Sichtprüfung im laufenden Desktop-Fenster; UI-Prüfungen erfolgten mit ImGui/ImPlot ohne Displayserver. SatDump und SDR-Hardware wurden nicht gestartet oder beendet.

**Annahmen**
- „Nicht horizontal“ bezieht sich auf die lange Inhalts-/Statuszeile; die native Tab-Leiste bleibt wie bei anderen Decodern oben.

## 2026-11-22 (5) — Codex

**Getan**
- Absturzsuche in Legacy-CLI auf Jans Wunsch abgebrochen; keine Änderungen an CLI-Dateien.
- Unter Settings → Plugin Settings → SDRPlay Support einen Enable/Disable-Knopf mit Status ergänzt. Er deaktiviert die SDRplay-Anbindung innerhalb von SatDump.
- Bei deaktivierter API überspringt die Gerätesuche SDRplay; neue API-Instanzen werden vor `dlopen` abgewiesen. Bereits geöffnete SDRplay-Quellen sperren den Knopf; zum Abschalten erst stoppen und eine andere Quelle auswählen.
- Einstellung wird über den vorhandenen Save-Mechanismus als `plugin_settings.sdrplay_enabled` gespeichert; bei fehlendem Schlüssel gilt weiterhin enabled.

**Geänderte Dateien**
- `plugins/sdr_sources/sdrplay_sdr_support/main.cpp`
- `plugins/sdr_sources/sdrplay_sdr_support/dynload.h`
- `plugins/sdr_sources/sdrplay_sdr_support/sdrplay_sdr.h`
- `plugins/sdr_sources/sdrplay_sdr_support/sdrplay_sdr.cpp`
- `HANDOFF.md`

**Verifiziert (per Befehl geprüft)**
- `cmake --build build -j4 --target sdrplay_sdr_support satdump-ui` erfolgreich.
- `git diff --check` ohne Befund vor diesem Eintrag.

**Offen**
- GUI-Klick, Speichern/Neustart und Sperre mit einer echten SDRplay-Quelle nicht interaktiv getestet; API-Bibliothek und Systemdienst in den geprüften Standardpfaden nicht gefunden.

**Annahmen**
- Auf die Rückfrage Systemdienst vs. lokale Anbindung lag bei Umsetzung keine Antwort vor. Nach angekündigter Annahme wurde nur die Anbindung in SatDump umgesetzt; der Knopf stoppt keinen systemweiten Dienst.

## 2026-11-22 (4) — Codex

**Getan**
- Editor-Build-Tasks und GDB-Starter verwenden einen gemeinsamen Wrapper: Nur bei vorhandener `/.flatpak-info` wird `flatpak-spawn --host` genutzt; sonst wird der Befehl direkt ausgeführt.
- Build-Tasks auf `cmake --build` umgestellt. Neuer UI-Task baut `satdump-ui` und `meteor_support`; die UI-Debug-Konfiguration verwendet diesen statt nur das Meteor-Plugin zu bauen.

**Geänderte Dateien**
- `.vscode/tasks.json`, `.vscode/launch.json`, `.vscode/gdb-host.sh`, neu `.vscode/host-exec.sh` (alle durch `.gitignore` ignoriert).
- `HANDOFF.md`.

**Verifiziert (per Befehl geprüft)**
- Neuer UI-Task-Befehl mit `/bin/sh .vscode/host-exec.sh cmake --build ... --target satdump-ui meteor_support -j$(nproc)` erfolgreich: beide Ziele gebaut.
- `.vscode/gdb-host.sh --version` startet GNU gdb 18.1 erfolgreich.
- Beide JSON-Konfigurationsdateien mit Python geparst; erfolgreich.

**Offen**
- Start über die Editor-Oberfläche und tatsächliche Debug-Sitzung nicht ausgeführt. Flatpak-Zweig in dieser Umgebung nicht ausführbar und nicht getestet.

**Annahmen**
- Keine Flatpak-/Distrobox-Marker in der geprüften Umgebung vorhanden; der direkte Build und GDB-Aufruf funktionieren dort.

## 2026-11-22 (3) — Codex

**Getan**
- In `RecorderApplication::drawMenu()` wird eine während der Dateiauswahl von File Source erkannte Frequenzänderung über `set_frequency()` an den Recorder weitergegeben. Dadurch werden Tunerreferenz, FFT-Skala und VFO-Marker aktualisiert, bevor `start()` die Recorderfrequenz erneut auf die Quelle setzt.
- Die bestehende Dateinamen-Erkennung in File Source bleibt erhalten; ohne erkannte Frequenzänderung bleibt die manuelle Referenz bestehen.

**Geänderte Dateien**
- `src-interface/recorder/recorder.cpp` (nur die Übergabe nach `drawControlUI()` ergänzt; bestehende Änderungen erhalten).
- `HANDOFF.md`.

**Verifiziert (per Befehl geprüft)**
- `cmake --build build -j4 --target satdump-ui recorder_frontend_test` erfolgreich.
- `ctest --test-dir build --output-on-failure -R '^recorder_frontend_test$'`: 1/1 bestanden. Der vorhandene Test deckt den neuen GUI-Ablauf nicht ab.
- `git diff --check` ohne Befund vor dem Protokolleintrag.

**Offen**
- Interaktive Prüfung: Datei mit Frequenzkennung auswählen, Tuner-/FFT-Anzeige prüfen und Wiedergabe starten. GUI wurde in dieser Sitzung nicht gestartet.

**Annahmen**
- Die gewünschte Frequenzkennung entspricht einem bereits von `wav::tryParseFilenameMetadata()` unterstützten Dateinamenformat; der konkrete Dateiname war im Screenshot nicht lesbar.

## 2026-11-22 (2) — Google Antigravity

**Getan**
- **ELEKTRO-xRIT/xRPT & GGAK-Pipelines sowie Viterbi-/Deframer-/OQPSK-Einrastfehler**:
  - **OQPSK-Resampling (`src-core/pipeline/modules/demod/module_psk_demod.cpp`, `resources/pipelines/Elektro_Arktika.json`)**:
    - `DelayOneImagBlock` verzögert Q um genau 1 Sample, was nur bei exakt `2.0 SPS` einer halben Symboldauer ($0.5\,T_{\text{sym}}$) entspricht. In `PSKDemodModule` wurde für `is_oqpsk` `MIN_SPS = 2.0; MAX_SPS = 2.0;` gesetzt und in `elektro_hrit` (`Elektro_Arktika.json`) das fehlerhafte `"max_sps": 3` entfernt (das zuvor z. B. bei 6 MS/s zu `2.61 SPS` und damit zu $0.383\,T_{\text{sym}}$ I/Q-Versatz vor der Taktrückgewinnung führte).
    - Costas-Loop-Bandbreite in `Elektro_Arktika.json` für `elektro_hrit` von `0.001` auf `0.003` angehoben und für `elektro_lrit` von `0.0145` auf `0.005` reduziert.
    - Die bewährten `elektro_ggak`-`pm_demod`-Parameter (`"pll_bw": 0.01`, `"pll_acquire_frequency": true`, `"clock_gain_mu": 0.05`, `"clock_gain_omega": 0.000625`) auf `elektro_ggak_mqtt`, `arktika_m1_ggak` und `arktika_m2_ggak` übertragen sowie den fehlenden `elektro_arktika_ggak_instruments`-Schritt in `arktika_m1_ggak` und `arktika_m2_ggak` ergänzt.
  - **`Viterbi1_2`, `CCDecoder` & `CCEncoder` (`src-core/common/codings/viterbi/*`)**:
    - Out-of-Bounds-Read von 12–13 Bytes über `ber_soft_buffer` in `ST_IDLE` durch Vergrößerung von `ber_test_buffer` und `ber_soft_buffer` um `TAIL_SYMBOLS + 4` (`TAIL_SYMBOLS = 12`) und Kopieren von `test_len` Symbolen behoben.
    - `reset()` in `CCDecoder` (`init_viterbi_unbiased(&d_vp)`) und `CCEncoder` (`d_start_state = 0`) ergänzt; in `ST_IDLE` wird jeder Kandidat `(iq_swap, phase, shift)` von einem unvoreingenommenen Trellis aus getestet und `cc_decoder` beim Einrasten zurückgesetzt.
    - In `ST_SYNCED` wird der Eingabepuffer nicht mehr in-place überschrieben (`rot_buffer`), bei `d_shift == 1` das ungerade Rand-Symbol über Puffergrenzen hinweg per `d_carry_symbol` übernommen (statt bei jedem 8192-Symbol-Block 1 Symbol zu verlieren) und der 12-Symbol-Lookahead-Tail (`d_tail_carry[12]`) blockübergreifend fortgeführt, sodass `cc_decoder.work` ohne Bitverlust an 4096-Bit-Grenzen durchläuft.
    - `get_ber()` überspringt die ersten 6 Bitpaare (`TAIL_SYMBOLS = 12`), während derer sich das Schieberegister von `CCEncoder` füllt.
  - **CCSDS-Deframer & Decoder (`bpsk_ccsds_deframer.cpp`, `module_ccsds_conv_concat_decoder.cpp`, `module_elektro_lrit_data_decoder.cpp`, `module_msugs.cpp`)**:
    - In `BPSK_CCSDS_Deframer::work()` wieder 1 Bitfehler im 32-Bit-ASM während `STATE_NOSYNC` (`compare_32(shifter, CADU_ASM) < d_state`) zugelassen.
    - In `CCSDSConvConcatDecoderModule`, `ElektroLRITDataDecoderModule` und `MSUGSDecoderModule` im `DATA_DSP_STREAM`-Pfad `if (!should_run()) break;` nach `read_data()` ergänzt (verhindert Hängen beim Stoppen von Live-Pipelines).
    - In `CCSDSConvConcatDecoderModule::getModuleStats()` `rs_avg` durch `d_rs_interleaving_depth` statt stets durch `4` geteilt.
    - In `MSUGSDecoderModule::process()` das Ausgabeverzeichnis vor dem Öffnen von `data_unknown.bin` angelegt und die tote 103-Millionen-Pixel-`printf`-Debugschleife deaktiviert (`#if 0`).
- **Checkbox `"DC correction"` nur für E4000-Tuner (`rtlsdr_sdr.cpp`, `pipeline_tuning.h`, `recorder.cpp`, `recorder_proc.cpp`)**:
  - `RtlSdrSource::open()` fragt den Tunertyp bereits beim Auswählen des Geräts über `rtlsdr_get_tuner_type()` ab und legt ihn in `d_settings["tuner_type"]` ab (wird in `set_settings()`, `get_settings()` und `start()` fortgeführt); auch der Offset-Tuning-Hinweis in `RtlSdrSource::drawControlUI()` ist damit bereits im gestoppten Zustand gerätespezifisch.
  - Neue Hilfsfunktion `satdump::recorder::is_e4000_source(desc, settings)` in `src-interface/recorder/pipeline_tuning.h` prüft auf `desc.source_type == "rtlsdr"` und `tuner_type == RTLSDR_TUNER_E4000` (`1`).
  - Die Checkbox `"DC correction"` (`dc_blocking`) wird in `RecorderApplication::drawMenu()` nur angezeigt, wenn `is_e4000_source(...)` zutrifft; für alle anderen Quellen/Tuner (z. B. R820T/R828D, HydraSDR, Airspy) wird `dc_blocking` in `drawMenu()`, `try_load_sdr_settings()` und `start()` auf `false` gezwungen.

**Geänderte Dateien**
- `src-core/pipeline/modules/demod/module_psk_demod.cpp`
- `src-core/common/codings/viterbi/cc_decoder.h`
- `src-core/common/codings/viterbi/cc_encoder.h`
- `src-core/common/codings/viterbi/viterbi_1_2.h`
- `src-core/common/codings/viterbi/viterbi_1_2.cpp`
- `src-core/common/codings/deframing/bpsk_ccsds_deframer.cpp`
- `src-core/pipeline/modules/ccsds/module_ccsds_conv_concat_decoder.cpp`
- `plugins/elektro_arktika_support/elektro_arktika/lrit/module_elektro_lrit_data_decoder.cpp`
- `plugins/elektro_arktika_support/elektro_arktika/instruments/msugs/module_msugs.cpp`
- `resources/pipelines/Elektro_Arktika.json`
- `plugins/sdr_sources/rtlsdr_sdr_support/rtlsdr_sdr.cpp`
- `src-interface/recorder/pipeline_tuning.h`
- `src-interface/recorder/recorder.cpp`
- `src-interface/recorder/recorder_proc.cpp`
- `tests/recorder_frontend_test.cpp`
- `HANDOFF.md`

**Verifiziert (per Befehl geprüft)**
- Ziele `satdump_core`, `satdump_interface`, `orbcomm_support`, `elektro_arktika_support`, `transit_support`, `rtlsdr_sdr_support`, `satdump`, `satdump-ui`, `recorder_frontend_test`, `transit_protocol_test`, `transit_pam_test`, `transit_live_test`, `transit_product_test` fehlerfrei gebaut.
- `ctest --test-dir build --output-on-failure -V` besteht komplett (inkl. Tests für E4000-Erkennung in `open()`/`is_e4000_source()` gegen R820T/HydraSDR sowie `test_viterbi_1_2_continuity` über 4 Blöcke für `shift = 0` und `shift = 1`).
- `./build/plugins/transit_protocol_test`, `./build/plugins/transit_pam_test` und `./build/plugins/transit_live_test` bestehen komplett.

**Offen**
- Keine offenen Punkte aus dieser Aufgabe.

**Annahmen**
- Keine.

## 2026-11-22 — Google Antigravity

**Getan**
- **Orbcomm STX (`module_orbcomm_stx_auto_demod.{h,cpp}`, `module_orbcomm_stx_demod.{h,cpp}`)**:
  - Out-of-Bounds-Read bei `0x65`-Sync-Paketen am Frame-Ende (`i == 49`, 24-Byte-FCS auf verbleibenden 12 Bytes) durch `i + 1 < 50` verhindert.
  - Data Race / Use-After-Free und unbegrenztes Anwachsen von `freqs_to_push` behoben (`freqs_to_push` wird unter `freqs_to_push_mtx` per `swap` entnommen und anschließend abgearbeitet).
  - Nyquist-/Bandbreitenprüfung (`std::isfinite(frequency) && std::abs(d_center_frequency - frequency) <= d_samplerate / 2.0`) in `add_stx_link` ergänzt.
  - Audio-Initialisierungs- und Shutdown-Races in `OrbcommSTXAutoDemodModule` und `OrbcommSTXDemodModule` durch `audio_mtx` sowie `std::atomic` für `enable_audio`, `play_audio` und `audio_freq` behoben (`enable_audio = true` erst nach Initialisierung von `audio_buffer` und `audio_sink->start()`, Pufferlängenbegrenzung auf `dsp::STREAM_BUFFER_SIZE`, synchronisiertes Stoppen in `stop()` und `process()`, Lock auf `stx_link_lst_mtx` und `freqs_to_push_mtx` in `OrbcommSTXAutoDemodModule::stop()`).
- **`PMDemodModule` (`src-core/pipeline/modules/demod/module_pm_demod.cpp`)**:
  - Bei `d_resample_after_pll == true` wird `resampler` nur angelegt, wenn `resample == true` ist (`agc2` liest sonst direkt `pm_psk->output_stream`), wodurch der Pipeline-Deadlock bei `resample == false` entfällt.
  - `display_freq` wird mit der tatsächlichen PLL-Abtastrate (`d_resample_after_pll ? d_samplerate : final_samplerate`) berechnet.
- **`PLLCarrierTrackingBlock` (`src-core/common/dsp/pll/pll_carrier_tracking.{h,cpp}`)**:
  - Grobe Frequenzakquisition (Lag-1-Autokorrelation) läuft nun **vor** der PLL-Schleife puffergrenzenübergreifend (`d_acquire_last`, `d_acquire_has_last`) und setzt `d_freq` unmittelbar beim Erreichen von `d_acquire_samples`, sodass bereits der erste Puffer mit der geschätzten Trägerfrequenz demoduliert wird.
- **Elektro/Arktika GGAK (`plugins/elektro_arktika_support/elektro_arktika/ggak/*`)**:
  - `GGAKInstrumentsDecoderModule::getModuleStats()` implementiert (`frames`, `crc_errors`, `skl_samples`).
  - `GGAKMerger::process()` (`merger.h`): `recovery_queue.erase(std::remove_if(...), recovery_queue.end())` ergänzt, damit der Merger aus `WAIT` wieder nach `SYNCED` wechselt.
  - `GGAKIngestor` (`ingestor.h`): `while (fin.read((char *)&frm, sizeof(frm)))` und `checkCRC(&frm)` vor `parseSKLRecord(&frm)` eingesetzt.
  - `parseSKLRecord` (`skl.cpp`): 1-Sekunden-Zeitstempel `getTimestamp(frm) + i` für die 14 Samples pro Frame vergeben.
- **TRANSIT 5E-5 & `BaseDemodModule` (`plugins/transit_support/*`, `src-core/pipeline/modules/demod/module_demod_base.cpp`)**:
  - In `BaseDemodModule::initb()` wird nach Bestimmung von `final_samplerate` `resample = final_samplerate != d_samplerate;` gesetzt (verhindert unnötigen 1:1-Resampler bei 64 kS/s und aktiviert Resampling bei abweichender `custom_samplerate`).
  - In `TransitDemodModule::process()` werden im `DATA_FILE`-Modus bei `file_source->eof()` noch in der DSP-Kette befindliche Puffer vollständig ausgelesen, bevor `preview.finish()` und `stop()` laufen; `TransitDemodModule::stop()` prüft Block-Pointer auf Null.
  - In `TuningSpectrum::peak()` (`tuning_spectrum.h`) werden `lo` und `hi` auf `[0, int(values.size())]` begrenzt.
- **`CCSDSSimplePSKDecoderModule` (`src-core/pipeline/modules/ccsds/module_ccsds_simple_psk_decoder.cpp`)**:
  - Modulo-Wraparound `% int(200 * ui_scale)` auf vorzeichenbehafteten Konstellationskoordinaten in `drawUI()` entfernt; `rs_avg` in `getModuleStats()` mittelt über `d_rs_interleaving_depth`.
- **Recorder-VFO-Verwaltung (`src-interface/recorder/recorder_vfo.cpp`)**:
  - `update_vfo_markers()` aktualisiert `last_preview_pipeline_id` bei jedem Rebuild (verhindert unnötige Rebuilds pro Frame bei laufender Pipeline und eingeklapptem Processing-Panel).
  - `add_vfo_reco()` normiert `decimation <= 1` auf `1` (verhindert Unterlauf durch Default `decimation = -1` bei `get_samplerate() / decimation`); Fehler-Cleanup in `add_vfo_live()` / `add_vfo_reco()` und Stop-Reihenfolge (`decim_ptr` vor `file_sink`) in `del_vfo()` bereinigt.

**Geänderte Dateien**
- `plugins/orbcomm_support/orbcomm/module_orbcomm_stx_auto_demod.h`
- `plugins/orbcomm_support/orbcomm/module_orbcomm_stx_auto_demod.cpp`
- `plugins/orbcomm_support/orbcomm/module_orbcomm_stx_demod.h`
- `plugins/orbcomm_support/orbcomm/module_orbcomm_stx_demod.cpp`
- `src-core/pipeline/modules/demod/module_pm_demod.cpp`
- `src-core/pipeline/modules/demod/module_demod_base.cpp`
- `src-core/common/dsp/pll/pll_carrier_tracking.h`
- `src-core/common/dsp/pll/pll_carrier_tracking.cpp`
- `src-core/pipeline/modules/ccsds/module_ccsds_simple_psk_decoder.cpp`
- `plugins/elektro_arktika_support/elektro_arktika/ggak/module_ggak_instruments.h`
- `plugins/elektro_arktika_support/elektro_arktika/ggak/module_ggak_instruments.cpp`
- `plugins/elektro_arktika_support/elektro_arktika/ggak/merger.h`
- `plugins/elektro_arktika_support/elektro_arktika/ggak/ingestor.h`
- `plugins/elektro_arktika_support/elektro_arktika/ggak/skl.cpp`
- `plugins/transit_support/module_transit_demod.cpp`
- `plugins/transit_support/tuning_spectrum.h`
- `plugins/transit_support/tests/live_test.cpp`
- `src-interface/recorder/recorder_vfo.cpp`
- `tests/recorder_frontend_test.cpp`
- `HANDOFF.md`

**Verifiziert (per Befehl geprüft)**
- Ziele `satdump_core`, `satdump_interface`, `orbcomm_support`, `elektro_arktika_support`, `transit_support`, `rtlsdr_sdr_support`, `satdump`, `satdump-ui`, `recorder_frontend_test`, `transit_protocol_test`, `transit_pam_test`, `transit_live_test`, `transit_product_test` gebaut.
- `ctest --test-dir build --output-on-failure -V` besteht komplett (inkl. neuem Regressionstest `test_pll_frequency_acquisition`).
- `./build/plugins/transit_protocol_test`, `./build/plugins/transit_pam_test` und `./build/plugins/transit_live_test` (inkl. neuem `DATA_FILE`-EOF-Drain-Regressionstest) bestehen komplett.

**Offen**
1. Falls auch die reinen Upstream-Fehler aus Teil B der Analyse (z. B. HydraSDR/Airspy `stopWriter()`, `BasebandType`-Komparator, `kepler_utils.cpp`, `orbit.c`, `flowgraph.cpp`) behoben werden sollen, können diese im nächsten Schritt umgesetzt werden.

**Annahmen**
- Keine.

## 2026-11-20 (2) — Google Antigravity

**Getan**
- Schritt 1: `tests/recorder_frontend_test.cpp` als optionales CMake-Target `recorder_frontend_test` hinter `SATDUMP_BUILD_TESTS` (Voreinstellung `OFF`) mit `add_test` in `CMakeLists.txt` eingebunden und `tests/recorder_frontend_test.md` auf den neuen CMake/CTest-Aufruf gekürzt.
- Schritt 2: `dsp::CorrectIQBlock` (`src-core/common/dsp/utils/correct_iq.{h,cpp}`) auf `double alpha`, `satdump_exception` (`std::runtime_error`), statische Hilfsfunktion `alpha_from_samplerate(samplerate, time_constant_seconds = 1.0 / 50.0)` und gemeinsame Inline-Funktion `dsp::correct_iq_sample` umgestellt; Recorder und Test nutzen `alpha_from_samplerate`. Alle bestehenden Nutzer (`module_demod_base`, `module_psk_demod`, `module_fsk_demod`, `module_sdpsk_demod`, `module_orbcomm_stx_demod`, `stx_demod_hier`, `module_lucky7_demod`) geprüft (unverändertes Standardverhalten mit `alpha = 0.0001`).
- Schritt 3: `satdump::ndsp::CorrectIQBlock` (`src-core/dsp/utils/correct_iq.{h,cpp}`) auf `double`-Zustand (`alpha`, `acc_i`, `acc_q`) und `dsp::correct_iq_sample` angeglichen; Flowgraph-Registrierungsname `"Utils/Correct IQ"` unverändert gelassen.
- Schritt 4: `dc_blocking` aus `serialize_config`/`deserialize_config` entfernt (alter globaler Schlüssel wird beim Laden toleriert, aber nicht mehr geschrieben) und stattdessen pro Quelle in `rec_cfg[<Quellenname>]["dc_blocking"]` mit Voreinstellung `false` in `try_load_sdr_settings()` / `stop()` verwaltet (Hilfsfunktionen `load_source_dc_blocking` und `save_source_dc_blocking` in `src-interface/recorder/pipeline_tuning.h`).
- Schritt 5: In `RecorderApplication::start()` wird jede `std::exception` gefangen und im Fehlerfall nach `source_ptr->start()` werden bereits gestartete Blöcke (`dc_blocker`, `decim_ptr`) und `source_ptr` sauber gestoppt und zurückgesetzt.
- Schritt 6: Tooltip der Checkbox `"DC correction"` um Hinweis auf Dämpfung von Signalanteilen exakt in Bandmitte (Restträger -> Frequenzversatz nutzen) ergänzt; E4000-Hinweistext in `rtlsdr_sdr.cpp` um Hinweis ergänzt, dass Offset Tuning bevorzugt ist und Software-DC-Korrektur ersetzt.
- Schritt 7: `RecorderApplication::update_vfo_markers()` prüft `if (!fft_plot) return;`, nutzt `std::unique_lock<std::mutex> lock(vfos_mtx, std::try_to_lock)` (blockiert den UI-Thread nicht, zeigt bei Misserfolg den letzten Stand weiter) und baut die Marker nur bei tatsächlicher Änderung (VFO hinzugefügt/entfernt, Pipeline gestartet/gestoppt, Pipeline-Auswahl/Parameter geändert, Frequenzwechsel) neu auf.

**Geänderte Dateien**
- `CMakeLists.txt`
- `tests/recorder_frontend_test.md`
- `tests/recorder_frontend_test.cpp`
- `src-core/common/dsp/utils/correct_iq.h`
- `src-core/common/dsp/utils/correct_iq.cpp`
- `src-core/dsp/utils/correct_iq.h`
- `src-core/dsp/utils/correct_iq.cpp`
- `src-interface/recorder/pipeline_tuning.h`
- `src-interface/recorder/recorder.h`
- `src-interface/recorder/recorder.cpp`
- `src-interface/recorder/recorder_proc.cpp`
- `src-interface/recorder/recorder_vfo.cpp`
- `plugins/sdr_sources/rtlsdr_sdr_support/rtlsdr_sdr.cpp`
- `HANDOFF.md`

**Verifiziert (per Befehl geprüft)**
- Ziele `satdump_core`, `satdump_interface`, `rtlsdr_sdr_support`, `orbcomm_support`, `cubesat_support`, `satdump-ui`, `satdump` und `recorder_frontend_test` gebaut.
- `ctest --test-dir build --output-on-failure -V` besteht komplett (inkl. Tests für `alpha_from_samplerate`, `ndsp::CorrectIQBlock` und Pro-Quelle-`dc_blocking` samt Toleranz des alten globalen Schlüssels).

**Offen**
1. Manuelle Hardware-/UI-Abnahme durch Jan (E4000 mit/ohne Offset Tuning, HydraSDR mit DC-Korrektur aus, RTL-SDR mit R820T, Stop/Start, Neustart mit geladenen Einstellungen, mehrere VFOs anlegen und einzeln löschen).

**Annahmen**
- Keine.

## 2026-11-20 — Google Antigravity

**Getan**
- CMake mit `PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig` neu konfiguriert (`cmake -B build`), sodass `HYDRASDR_LIBRARY=/usr/local/lib64/libhydrasdr.so` im Cache gefunden wurde.
- Ziel `hydrasdr_sdr_support` (wobei `satdump_core` wegen geänderter Kerndateien mitgebaut wurde) sowie `satdump` (CLI-Binary im Build-Ordner) ohne Installation gebaut.
- Herkunft der Installation unter `/usr` per Zeitstempel und `sha256sum` gegen `~/SatDump/build` und `~/SatDump2/SatDump/build` geprüft.

**Geänderte Dateien**
- `HANDOFF.md` (dieser Eintrag).
- Innerhalb von `build/`: `build/CMakeCache.txt`, `build/plugins/libhydrasdr_sdr_support.so`, `build/libsatdump_core.so`, `build/satdump` (+ zugehörige Objektdateien). Keine Änderungen an getrackten Quelldateien.

**Verifiziert (per Befehl geprüft)**
- `ldd build/plugins/libhydrasdr_sdr_support.so` löst `libhydrasdr.so.1 => /usr/local/lib64/libhydrasdr.so.1` und `libsatdump_core.so.2.0.0 => /home/jankoch/SatDump/build/libsatdump_core.so.2.0.0` fehlerfrei auf.
- `./satdump legacy sdr_probe` aus `build/` erkennt und listet `HydraSDR One 36b463dc359439c7` (type: `hydrasdr`, id: `3941885371298888135`).
- Vergleich `/usr/lib64/libsatdump_core.so.2.0` (`mtime` 2026-10-25 23:16:25, `sha256` `9f848b68...`): wurde laut `build/install_manifest.txt` (`mtime` 2026-10-25 23:21:14) ursprünglich aus `~/SatDump/build` installiert, entspricht per `sha256sum` aber keinem der beiden aktuellen Build-Verzeichnisse mehr (da `~/SatDump/build/libsatdump_core.so` am 28.10.2026 und heute neu gebaut wurde; `~/SatDump2/SatDump/build` stammt vom 15.09.2026, `sha256` `d3f68eba...`).
- `git diff` vor und nach dem Build ist byte-identisch (keine Quelländerungen).

**Offen**
1. Falls `satdump-ui` oder andere Plugins aus `build/` genutzt werden sollen, die gegen `libsatdump_core.so` linken, ggf. `satdump-ui` bzw. die Plugins bei Bedarf ebenfalls neu linken.
2. Eigene Änderungen weiterhin uncommittet (siehe Punkte 3 und 4 im vorherigen Eintrag).

**Annahmen**
- Keine.

## 2026-11-20 — Claude (Chat, Fernzugriff auf den Ordner)

**Getan**
- `AGENTS.md`, `CLAUDE.md` (nur Verweis) und diese Datei angelegt. Keine Git-Operation,
  kein Build, keine Quelländerung.

**Geänderte Dateien**
- Neu: `AGENTS.md`, `CLAUDE.md`, `HANDOFF.md` (alle untracked).

**Verifiziert (per Befehl geprüft)**
- HEAD `9fee0793c`, Branch `master`; 24 geänderte Dateien (533 Einfügungen, 84 Löschungen)
  plus untracked `plugins/transit_support/`, `tests/`, Transit-Ressourcen, `tap.cpp/.h`,
  `pipeline_tuning.h`.
- `build/CMakeCache.txt`: Präfix `/usr`, Release, `PLUGIN_HYDRASDR_SDR_SUPPORT=ON`,
  `HYDRASDR_LIBRARY=HYDRASDR_LIBRARY-NOTFOUND`; noch kein `hydrasdr_sdr_support`-Plugin in `build/`.
- `build/install_manifest.txt` datiert vom 25.10.2026 22:21 und führt `/usr/bin/satdump`.

**Offen**
1. HydraSDR-Plugin nachbauen (ohne Installation): an Antigravity vergeben, Stand unbekannt.
2. Herkunft der Installation unter `/usr` klären: Das Manifest belegt eine Installation aus
   diesem Baum am 25.10.2026; ob `/usr` heute diesem Stand, `~/SatDump2` oder einem
   unmodifizierten Stand entspricht, ist nicht geprüft (Prüfsummenvergleich steht aus).
3. Eigene Änderungen sind ungesichert. Vorschlag, nicht umgesetzt: lokaler Branch
   (z. B. `jan/mods`) mit Commit des jetzigen Stands; Entscheidung liegt bei Jan.
4. `.claude/settings.local.json` erlaubt `Bash(git checkout *)`; das widerspricht den
   Verboten in `AGENTS.md` und kann uncommittete Änderungen vernichten. Prüfen und ggf. entfernen.

**Annahmen**
- Antigravity liest `AGENTS.md` zusätzlich zu `.agents/rules/`; nicht in dieser Sitzung getestet.
