# HANDOFF — Übergabeprotokoll

Neuester Eintrag oben. Format je Eintrag: Datum, Werkzeug, Getan, Geänderte Dateien,
Verifiziert, Offen, Annahmen.

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
