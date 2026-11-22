# HANDOFF — Übergabeprotokoll

Neuester Eintrag oben. Format je Eintrag: Datum, Werkzeug, Getan, Geänderte Dateien,
Verifiziert, Offen, Annahmen.

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
