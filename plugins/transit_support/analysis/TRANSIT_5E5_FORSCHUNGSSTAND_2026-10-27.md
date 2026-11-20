# TRANSIT 5E-5: gemeinsamer Forschungsstand

Stand 27. Oktober 2026. Diese Notiz bündelt Messungen, Decoder-Ergebnisse, historische Quellen und offene Fragen. Die fünf Archiv-/FOIA-Mails an Navy FOIA, JHU/APL, NASA NSSDCA, NHHC und NARA sind nach Auskunft des Nutzers versandt; Antworten liegen in dieser Arbeitsumgebung noch nicht vor. Die [Mailtexte](TRANSIT_READY_EMAILS.md) enthalten die genauen Anfragen. **Messung**, **Dokumentbeleg** und **Interpretation** sind unten getrennt; die eingehenden Antworten sollen den Status der Hypothesen ändern, nicht frühere Vermutungen stillschweigend zu Fakten machen.

## 1. Was wir tatsächlich empfangen und dekodieren

| Ebene | Gesicherter oder unmittelbar geprüfter Befund | Grenze |
| --- | --- | --- |
| Objekt | Vier eigene Aufnahmen vom 3./4. Oktober 2026 passen per Doppler zu **NORAD 965 / 1964-083D**; der Nutzerbericht gibt Restfehler 8–28 Hz an. | Das ordnet den Sender einem Bahnkörper zu, nicht zweifelsfrei einer historischen Nutzlastbezeichnung. |
| Historische Identität | NASA-Tabellen und NSSDCA nennen **Transit 5E-5 = 1964-083C / NORAD 959**, **Oscar 2 / Transit 5B-5 = 1964-083D / NORAD 965**. 5E-5-Berichte beschreiben die zum Signal passende 136,65-MHz-Wissenschaftstelemetrie. | Die Behauptung, die physischen C/D-Satelliten seien vertauscht, bleibt zu prüfen. Technische Signalähnlichkeit und Dopplerfit sind keine Seriennummernurkunde. |
| HF | Träger nominell 136,650 MHz, im aktuellen Signal um 136,6535 MHz; Phasenmodulation mit einem analogen Unterträger um 5,45 kHz und einem digitalen FSK-Unterträger um 10,6 kHz. FSK-Töne etwa 9,95/11,25 kHz. | Frequenzen/Offset sind empfangs- und aufnahmespezifisch, keine Kalibrierung der historischen Sendernennwerte. |
| PCM 2026 | Gemessen **390,594 bit/s**, 256 Bit je Subframe, 16 Wörter à 16 Bit; alternierende Syncwörter `E240`/`E25F`. Zwei Subframes dauern zusammen etwa 1,31 s. | Kein CRC. Passende Sync- und ADC-Formate beweisen nicht, dass jedes Datenbit richtig ist. |
| PAM 2026 | Analoger Rahmen etwa **12,453 s**: ca. 1,31 s Syncplateau, dann 34 Datensegmente à ca. 0,3277 s. Die Segmente 2/30 sind stabile negative/positive Kalibrierplateaus; Segment 8 trägt eine reproduzierbare mehrstufige Kurve im ~20-ms-Raster. | Die physikalischen Kanalnamen und die einzelnen Pulspositionen von Segment 8 sind nicht bewiesen. |
| Fremdaufnahme vor 2023 | Ein von Happysat stammender Mono-Tonmitschnitt zeigt eine unabhängig messbare Wiederholung der analogen Tonfolge nach rund **12,43–12,45 s**; [Messung und Herkunft](TRANSIT_EXTERNAL_AUDIO_2014.md). | Kein IQ, keine gesicherte Frequenz-/Zeitangabe und keine bitgenaue PCM- oder C/D-Identitätsprüfung. |

Die vollständigen lokalen Messnotizen sind [Signalwissen](/home/jankoch/Dokumente/recv/transit_decoded/TRANSIT_5E5_Signalwissen.md) und [PAM-Auswertung](/home/jankoch/Dokumente/recv/transit_decoded/TRANSIT_5E5_PAM.md). Sie enthalten auch frühere Interpretationen, die durch spätere Prüfungen eingeschränkt wurden; bei Widerspruch gilt der vorsichtigere Stand dieser Zusammenfassung.

## 2. Was der SatDump-Decoder bereits leistet

Der experimentelle Plugin-Code und seine Bedienung sind in [README](../README.md) dokumentiert. IQ wird für PCM/PAM getrennt demoduliert; die Live-Ansicht bietet Feinabstimmung, PCM-Ton-/HF-Anzeige, Statuslampen in einer Zeile, PAM-Anzeige und Dateiexporte. Gespeicherte Produkte enthalten die Rohwörter und interpretierte Wörter, Kommutatorwerte, Events und analoge PAM-Segmente. Die beiden Zweige bleiben zeitlich getrennt, bis eine belastbare Flanken-Ausrichtung vorliegt.

Ein Replay des mitgelieferten 3.-Oktober-Ausschnitts lieferte **215 angenommene PCM-Subframes** (204 H, 11 X) und **9 angenommene analoge PAM-Rahmen**. Der Vergleich mit der älteren PAM-Segment-CSV hatte über 279 Werte einen medianen absoluten Unterschied von 4,38 Hz. Das ist ein Reproduktionstest an derselben Aufnahme, keine unabhängige physikalische Kalibrierung. In der [Statusprüfung](../../../build/transit_status_audit/report.md) stimmen 1128 von 1133 exportierten Frames nach deklarierter Rotation vollständig mit einem passenden Rohkandidaten überein; das bestätigt den Verarbeitungspfad, nicht die Fehlerfreiheit des Funksignals.

**H/R/X sind Qualitäts-/Formatklassen des Decoders, keine belegten Raumfahrzeug-Betriebszustände.** H bedeutet passende ADC-Formatbits, R passend nach einer Wortrotation, X keine dieser beiden Formen. Zählerkontinuität wird separat betrachtet. Ein Zählersprung ist weder automatisch ein Neustart noch ein Beweis für einen defekten Encoder. Die Zählerzeit ist modulo 65536 und kein nachgewiesener Betriebsstundenzähler.

## 3. Feldbelegung: Messung versus Bedeutung

| Feld | Wiederholte Beobachtung | Belastbare Bedeutung heute |
| --- | --- | --- |
| w1 | Alternierende Syncmuster `E240`/`E25F`. | Subframe-Synchronisierung. |
| w2/w6/w10/w14 | 12-Bit-Werte eines periodischen 42-Werte-Stroms; stabiler Marker um 1642–1652 Counts ermöglicht Nummerierung nach zwei Zyklen. CH12/13, 14/15, …, 24/25 bilden in zwei Durchgängen sieben nahezu identische Nachbarpaare (Median der absoluten Paardifferenz je 0–3 Counts). | Digitaler Kommutatorstrom. Falls jedes Paar eine doppelte Abtastung derselben Quelle ist, bleiben genau **35 unterschiedliche Quellen**; das passt auffällig zur historischen 35-Kanal-Angabe. Die physikalische Belegung und das Wiederholungsprinzip sind trotzdem nicht bewiesen. |
| w7/w15 | `01 + 10-Bit-Wert + 0001`; folgt dem analogen PAM-Hub. | Digitaler Abgriff des analogen PAM-Signals; relative ADC-Werte, keine technische Einheit. |
| w9 | Zählt in den brauchbaren H-Paaren in der beobachteten Subframe-Folge. | Rahmenzähler-Kandidat; absolute Zeitepoche offen. |
| w12 | Mehrere Bits wechseln und bilden Treppen. | Keine zuverlässige Bit- oder Kanalbedeutung. Einfache Binär-/Gray-Deutungen erklären die Durchgänge nicht. |
| w13 b15…b10 | Bitumgekehrtes Gray-Feld von **mindestens sechs Bits** ist ein guter empirischer Kandidat: u. a. 54→…→63→0→…→7 am 3.10. und 55→…→36 am 4.10. | Wahrscheinlich ein abgestufter Lage-/Sonnenfeldwert, aber Sensor, Feldbreite (7 Bit nicht ausgeschlossen), Skala, Nullrichtung und Polarität offen. Frühere feste Vier-Bit-/16-Stufen-Etiketten sind zu eng. |
| w13 b9…b0 | Verschiedene langsame oder sporadische Wechsel. | Nicht als unabhängige Schaltflags identifiziert. |
| PAM Segment 8 | Wiederkehrende Mittel-, positive und negative Pegel mit etwa 20-ms-Struktur. | Status-/Telltale-Kanal ist plausibel, aber individuelle Pulse und Schalter sind **nicht** zugeordnet. |

Für den gemeinsamen analogen Inhalt von PAM und w7/w15 gibt es einen starken Test innerhalb eines Überflugs: 203 Anpassungspaare und 159 spätere Prüfpaarungen ergaben Spearman ρ≈0,993 bzw. 0,991; aus dem ersten Teil folgt näherungsweise `ADC = 301,89 + 0,2373 × Hub[Hz]`, mit 1,38 Counts medianem Fehler im späteren Teil. Der Zeitversatz ist wegen breiter Segmentplateaus nur ungefähr (nahe gleich gute Werte −175 bis −90 ms); daraus **keine** individuelle 20-ms-Puls-/PCM-Bit-Zuordnung ableiten. Quelle: [Audit-Report](../../../build/transit_status_audit/report.md).

Die Kanalhypothesen aus den Aufnahmen bleiben Hypothesen: digitale Kanäle 18/19 als Hauptsolarstrom, 38 als analoger Sonnenwinkel; PAM 5/6 als gegenläufiges Sonnensensorpaar, Segment 7 als Versorgungsspannung. Sonnen- und Temperaturkorrelationen können mehrere Kanäle gemeinsam treiben. Eine identische Kurve ist noch kein Kabelplan. Die als leer/rauschend beobachteten science-nahen Register bzw. PAM-Segmente beweisen nicht, dass sämtliche historischen Instrumente heute physisch abgeschaltet oder defekt sind.

Ein getrennter [Quellenindex mit Abgleich](TRANSIT_5E5_QUELLENINDEX_2026-10-27.md) unterscheidet Hardwarebeleg, Messsignal und Vermutung. Das [bewusst spekulative Guess-Modell](TRANSIT_5E5_GUESSMODELL.md) hält übergreifende Hypothesen und Widerlegungstests fest. Die [überarbeitete Kanal-Arbeitskarte](TRANSIT_5E5_KANALRATEN.md) benennt alle 34 PAM-Segmente und alle 42 digitalen Ausgabeplätze (vermutlich 35 Quellen), nennt aber bei schwachen Engineering-Namen konkurrierende Wissenschafts-/Bauteilfamilien. Keine dieser Notizen ändert Bezeichnungen oder Werte im produktiven Decoder.

## 4. Was die historischen Quellen wirklich hinzufügen

| Quelle | Konkreter Gewinn | Was sie nicht liefert |
| --- | --- | --- |
| [TG-890 / AD652807](/home/jankoch/Dokumente/recv/AD652807.pdf), Radford 1967 | Rb-85-Magnetometer, drei Auslegerschalter SW-1/2/3 und deren **analoge** historische Telemetriepegel ungefähr −250, −130, +250, +200 mV; Messgate 0,0819265 s alle 0,65536 s und Feld-Konversionsformel. | Keine Zuordnung dieser Pegel zu heutigem PAM-Segment 8 oder PCM w12/w13; kein heutiger Boomzustand. |
| [SDO-1600](/home/jankoch/Dokumente/recv/SDO1600.pdf), APL 1978 | 5E-5-Architektur mit drei 35-Kanal-Kommutatoren, teilweiser analoger Unterkommutierung und 256-Bit-Digitalencoder; ausdrücklich **analoge dreiachsige** und **digitale** Sonnenlageerfassung. Außerdem Rb-/Fluxgate-Magnetometer, UV, Teilchen-, Sublimations- und Bauteilversuche; acht Ein/Aus-Kommandos; Symposium „Results from the 5-E Series of Satellites“, 27.9.1974. | Keine Kanal-/Bitpositionstabelle. Die 42 Werte des heutigen digital abgeleiteten Stroms dürfen nicht stillschweigend mit einem der historischen 35-Kanal-Kommutatoren gleichgesetzt werden. |
| [Smith, NTRS 19660014433](https://ntrs.nasa.gov/citations/19660014433), 1965 | Zwei UV-Photometer, je 15 Bit **digitale** Kapazität, eine Messung alle 1,31 s; historische PCM-Rate **195,3 bit/s**. Personen: H. P. Lie (Elektronik), H. D. Black/G. B. Bush (Lage), M. W. Jennings (Datenreduktion). | Kein Wortindex für die Photometer und keine Erklärung für den Unterschied zur heutigen ~390,6-bit/s-Messung. PAM-Nahe-Null-Kanäle sind nicht automatisch UV-Messwerte. |
| [5E-1-Bericht, NTRS 19660004280](https://ntrs.nasa.gov/citations/19660004280) | Vorgängerarchitektur und Bodenverarbeitung: Pulse Data Processor → IBM 1401 → IBM 7094 Dictionary I/II. Acht Telltales im Vorgängerregister als Suchhinweis. | 5E-1 ist 1963-38C, nicht 5E-5; Register-/Kanalbelegung nicht übertragen. |
| [Fischell et al., Bergen 1965](TRANSIT_5E5_QUELLENINDEX_2026-10-27.md) | Inventar von 1964-83C-Degradationsexperimenten: Transistor-β 1–9, Transistorleckströme 10–18, Kondensatorleckstrom 19 und Kurzschlussversuche 20–23; Solarzellenversuche. | Experimentnummern sind keine gesicherten PCM-/PAM-Kanalnummern. Ehemaliger lokaler Buch-Download liegt zurzeit nicht am alten Pfad. |
| [NSSDCA-Katalog 64-083C-01A](https://spdf.gsfc.nasa.gov/pub/documents/old/documentation_from_nssdc/renamed_dataset_catalogs/transit5e5_mag_64-083c-01a_DSC_0032.pdf) | Verarbeiteter Magnetometerdatensatz `D000242`, restauriert als `DR004385` / `DS004385`; 1333 Datensätze à 80 Zeichen, APL/IBM-7094-Provenienz. | Reduzierte Messwerte sind nicht die rohe Telemetrie oder deren Kanalbuch. |
| [Hamblen/Oakes, *Electronics* 1961](TRANSIT_5E5_QUELLENINDEX_2026-10-27.md) | Direkter Einblick in **Transit 1B/2-A**-Telemetrie; nennt C. A. Blackburn und F. K. Preikschat als Entwickler eines damaligen Encoders. | Der 10,5-kHz-PDM-Kanal des älteren Transit 2-A belegt weder 5E-5-FSK noch dessen Statusbitbelegung; alter lokaler Download derzeit nicht verfügbar. |
| [Hermenau, Satellitenwelt](https://www.satellitenwelt.de/transit_5b-5.htm), Empfang ab 2002 | Unabhängige SSB-Beobachtung bei 136,650 MHz mit Seitenbändern um ±5,4 und ±10 kHz, analogem Muster alle etwa 12,5 s und separaten Tonbeispielen; [Abgleich](TRANSIT_EXTERNAL_AUDIO_2014.md). | Keine präzise Periodenmessung, Roh-IQ-Daten, Kanal-/Bitbelegung oder beweiskräftige Seriennummernzuordnung. Hypothesen über Batterie/Lage/RTG nicht als Messwerte übernehmen. |
| [Xerbo, *Investigating Transit – Part 1*](https://xerbo.net/posts/investigating-transit-pt1/), 2021 | Eigene Raw-I/Q-Empfänge; beschreibt zwei Unterträger, digitales FSK bei grob 398 bit/s, eine 256-Bit-Wiederholung und sichtbare Zählerstruktur. Sein GNU-Radio-Weg (HF-Demodulation → Frequenzverschiebung → Tiefpass → FSK-Demodulation) ist ein unabhängiger Plausibilitätsvergleich für unseren PCM-Zweig. | Die Bitrate ist nur grob angegeben; keine Wort-/Statusbit-Tabelle und keine herunterladbare IQ-Aufnahme auf der Artikelseite. Die Aussage, Zähler und Encoder würden „zufällig“ aussetzen, trennt Empfangsfehler, Demodulatorverlust und Bordfehler nicht. |

Weitere gelesene Quellen zu AN/UYK-1, AN/SRN-9, operationellem 150/400-MHz-Transit, S-66-Telltales und dem späteren TRIAD-RTG sind **Vergleichs- oder Ausschlussquellen**. Besonders der S-66-Bericht zeigt, wie eine Telltale-Tabelle aussehen *kann*; seine Pulspositionen, Pegel und Polarität dürfen nicht ohne 5E-5-Beleg übernommen werden. Details stehen in [Primärdokumenten-Befunden](TRANSIT_PRIMARY_DOCUMENT_FINDINGS.md), [Smith/Danchik/Potemra](SMITH_DANCHIK_POTEMRA_FINDINGS.md), [S-66-Befunden](S66_DOCUMENT_FINDINGS.md) und [Archivspuren](TRANSIT_5E5_ARCHIVE_REQUESTS.md).

## 5. Unaufgelöste Widersprüche, die Antworten entscheiden sollen

1. **C/D-Identität:** Doppler passt zum Katalogobjekt 83D/965, historische 5E-5-Publikationen ordnen ihre Nutzlast 83C/959 zu. Nötig: Original-Start-/Integrationsliste oder APL-Zeichnung mit S/N, Senderfrequenz und COSPAR-/NORAD-Zuordnung; gegebenenfalls 1964–65 Ground-Station-Log.
2. **195,3 versus 390,6 bit/s:** Smith belegt die historische Photometerübertragung bei 195,3 bit/s; die vier 2026-Aufnahmen zeigen 390,594 bit/s. Ob es eine spätere Umschaltung, eine historische Angabe für nur einen Datenmodus oder eine andere Zuordnung ist, bleibt offen. Decoder-Parameter dürfen den historischen Modus nicht stillschweigend ausschließen.
3. **Drei 35-Kanal-Kommutatoren versus 42 digital beobachtete Werte:** Sieben benachbarte Doppelwerte in beiden untersuchten Aufnahmen machen 35 unterschiedliche Quellen plausibel (42−7). Ob die Werte auf wiederholte Kontakte, Sample-and-hold oder Encoderduplikation zurückgehen und welchem der drei Kommutatoren sie entsprechen, bleibt offen. Nötig: Flugformat, Steuer-/Subkommutierungsplan und Ground-Decommutator-Dictionary.
4. **Status und Ausleger:** TG-890 dokumentiert einen analogen vierstufigen Auslegerstatus; PAM Segment 8 zeigt heute wiederholte Dreipegel-Impulse. Ein gemeinsamer Kanal oder eine bestimmte Schalterbelegung ist noch nicht gezeigt. Nötig: Telltale-/PAM-Kanalplan, Schaltbild und Flankenvergleich über unabhängige Passagen.

## 6. Archivspur und Eingangskontrolle

Die versandten Mails sind getrennte Suchwege, keine bereits erhaltenen Nachweise:

| Empfänger | Worauf eine Antwort sofort zu prüfen ist |
| --- | --- |
| Navy FOIA | Fall `DON-NAVY-2023-015194`: Antwortschreiben, Anlagenliste, wirklich ausgelieferte Dateien. Der öffentliche Log besagt „Full Release / Documents Sent“, aber nicht, **welche** Unterlagen. |
| JHU/APL | Flug-Encoderformat, PCM-Wortbelegungen, drei PAM-Kommutatoren, acht Kommandos/Telltales, Schaltzeichnungen, Status-Wahrheitstabellen; Seriennummern-/Frequenzliste; Dateien der Telemetry and Command Group und der genannten Ingenieure. |
| NASA NSSDCA | Restauriertes `D000242` samt Record-Layout, Variablendefinition und Verarbeitungshinweisen; Hinweise auf niedrigere Datenstufen oder Original-Dictionaries. |
| NHHC | Bestand, Findbuch, Signatur, Aktenordner oder konkrete Weiterleitung zum ursprünglichen Marine-/APL-Projekt. |
| NARA Archives II | RG 402 `UD-UP 134 / ID 341709`, technisches Berichtsregister/Box-Folder-Liste und Abgleich mit Dispositionseintrag 34; zunächst nur Fundstelle, bei identifiziertem gesperrtem Dokument gezielte Prüfung. |

Bei jeder Antwort zuerst **Dokumenttitel, Datum, Projektnummer, Raumfahrzeug-/Seriennummer, Frequenz, tatsächlich beschriebene Betriebsphase und Seiten mit Kanal-/Bitbelegung** erfassen. Von einem bloßen Transit-Titel nicht auf 5E-5 schließen. Ein Originalwortplan mit Statuslegende wäre der entscheidende Fund; ein verarbeiteter Messwertdatensatz oder eine Navigationsanleitung kann ihn nicht ersetzen.

## 7. Bis dahin die lohnendsten lokalen Prüfungen

1. Alle verfügbaren 2026-Passagen mit identischen Parametern exportieren und pro Feld/Bit Originalwort, Rotation, Syncqualität und Zählerqualität gemeinsam aufheben. Das schützt vor Scheinkorrelationen.
2. Die PCM/PAM-Zeitlage an **Flanken** kalibrieren und für PAM Segment 8 wiederholte Pulspositionen mit w12 sowie w13 b9…b0 vergleichen; stabile Negativbeispiele dokumentieren. Die bisherige Plateau-Korrelation reicht nur für den gemeinsamen ADC-Inhalt.
3. Den sechs- bis siebenbittigen Gray-Kandidaten über weitere, möglichst volle Überflüge und Sensor-Phasen vergleichen; keine Winkelgrade oder physikalische Achse ohne Kalibrierung ausgeben.
4. Rohkanäle 18/19, 38 und PAM 5/6/7 gegen Beleuchtung, Empfangs-/RF-Ausfälle und unabhängige Passagen testen. Mehrere gleich getriebene Kanäle nicht allein nach Korrelation benennen.
5. Falls frühe 1964/65-Rohdaten auftauchen: 195,3-bit/s-Modus separat demodulieren und TG-890s Rb-Zero-Crossing-Zahlen sowie Smiths 15-Bit-Photometerkadenz als numerische Prüfer verwenden.
