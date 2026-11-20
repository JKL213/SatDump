# AGENTS.md — gemeinsame Regeln für alle Coding-Agenten

Gilt für Claude Code, Google Antigravity und Cursor. Diese Datei ist die einzige
Quelle für Projektregeln; werkzeugspezifische Dateien (CLAUDE.md, GEMINI.md,
.cursor/rules/) enthalten höchstens einen Verweis hierher.

Architektur, Verzeichnisstruktur und Testziele: siehe `.agents/rules/project-overview.md`.

## Was dieser Baum ist

- Persönlich modifizierter SatDump-Arbeitsbaum von Jan (Fedora 44, x86_64).
- Basis: Upstream-Commit `9fee0793c` (22.10.2026), Branch `master`.
- Eigenständiger Arbeitsfork: wird nie mit dem Upstream synchronisiert und nicht dorthin
  beigetragen. Upstream-Kompatibilität ist kein Entwurfsziel; Kernklassen dürfen geändert werden.
- Enthält zahlreiche **uncommittete** eigene Änderungen (u. a. GGAK/Elektro-Arktika,
  Orbcomm STX, PLL, IQ-Korrektur, RTL-SDR, Recorder) sowie untracked eigene Arbeit
  (`plugins/transit_support/`, `tests/`, Transit-Pipeline und -Instrumentenconfig).
  Diese Änderungen sind nirgends sonst gesichert.

## Harte Verbote

1. Keine Installation: kein `cmake --install`, kein `make install`, kein `sudo`.
   Das Build-Verzeichnis ist auf `CMAKE_INSTALL_PREFIX=/usr` konfiguriert; eine
   Installation würde die systemweite SatDump-Version überschreiben.
2. Kein `git pull`, `merge`, `rebase`, `checkout <pfad>`, `restore`, `stash`,
   `reset`, `clean`. Kein Commit ohne ausdrückliche Freigabe durch Jan.
3. `build/` nicht löschen und `build/CMakeCache.txt` nicht verwerfen; nur gezielt
   einzelne Cache-Variablen per `-D` setzen.
4. Keine Schreibzugriffe außerhalb dieses Repos (Ausnahme: `/tmp`).
5. Bestehende Änderungen an Quelldateien nicht zurücknehmen oder "aufräumen".
   Neue Quelländerungen nur für die jeweils beauftragte Aufgabe.

Erfordert eine Aufgabe einen dieser Schritte: anhalten, begründen, nachfragen.

## Bauen und Ausführen

- Konfigurieren (Cache bleibt erhalten): `cmake -B build`
- Einzelnes Ziel bauen: `cmake --build build -j$(nproc) --target <ziel>`
- Ausführen direkt aus `build/`: `./satdump`, `./satdump-ui`; Plugins werden von dort geladen.
- Formatierung: `.clang-format` im Wurzelverzeichnis.

## Umgebung

- `libhydrasdr` 1.1.4 liegt unter `/usr/local` (Bibliothek `/usr/local/lib64`,
  Header `/usr/local/include/libhydrasdr`, pkg-config `/usr/local/lib64/pkgconfig`).
  Vor dem Konfigurieren: `export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig`.
- Systemweite SatDump-Installation unter `/usr` (`/usr/bin/satdump`, im PATH):
  nicht anfassen. Ein bloßes `satdump` startet diese, nicht den Build hier.
- Zweiter, älterer Baum: `~/SatDump2/SatDump` — nicht Gegenstand dieses Repos.
- Läuft ein Agent in einem Container (Flatpak, Distrobox), müssen Build- und
  Gerätebefehle auf dem Host ausgeführt werden (z. B. `flatpak-spawn --host`).
- SDR-Hardware kann nur von einem Programm zugleich geöffnet werden; laufende
  Programme (SDR++, SatDump) nicht eigenmächtig beenden.

## Übergabe zwischen Agenten

Es arbeitet immer nur ein Agent zur selben Zeit in diesem Baum.

- Sitzungsbeginn: `AGENTS.md` und `HANDOFF.md` lesen, `git status --short` prüfen.
- Sitzungsende: `HANDOFF.md` fortschreiben (neuester Eintrag oben): Datum, Werkzeug,
  was getan wurde, geänderte Dateien, was verifiziert ist, was offen ist,
  unbelegte Annahmen.
- Nichts als erledigt melden, was nicht durch einen ausgeführten Befehl belegt ist.
