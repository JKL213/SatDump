# Geratene Kanalbelegung für die 2026-TRANSIT-Mitschnitte

**Arbeitskarte, keine dokumentierte Telemetrietabelle.** Die Nummern entsprechen ausschließlich der heutigen Decoder-Ausgabe: PAM `seg1`–`seg34`, digital `ch0`–`ch41`. `S` = Struktur/Messverhalten stark gestützt, `M` = physikalische Familie aus Kurvenform und Hardwareinventar geraten, `W` = konkrete Funktion schwach geraten. Alle Rohwerte bleiben unskaliert; Namen wie „Spannung“, „Strom“ und „Temperatur“ sind keine Kalibrierung. Daten: PAM vom 3.10.2026, digitale Kurven vom 3. und 4.10.2026. Historische Belege und ihre Grenzen stehen im [neu indizierten Quellenabgleich](TRANSIT_5E5_QUELLENINDEX_2026-10-27.md). **Ein Schrägstrich zwischen Gerätefamilien bezeichnet konkurrierende Hypothesen, keine Mehrfachfunktion.**

## Digitaler Kommutator: 42 Ausgaben, wahrscheinlich 35 verschiedene Quellen

Die Paare 12/13, 14/15, 16/17, 18/19, 20/21, 22/23 und 24/25 unterscheiden sich in **beiden** ausgewerteten Aufnahmen im Median nur um 0–3 ADC-Counts. Keines der anderen benachbarten Paare zeigt diese durchgängige Nähe. Werden die sieben Paare als je eine Quelle mit Doppelabtastung gelesen, gilt **42 Ausgabeplätze − 7 Wiederholungen = 35 Quellen**. Das passt genau zu den historischen 35-Kanal-Kommutatoren. Es könnte auch eine Wiederholung im Encoder oder ein Sample-and-hold-Effekt sein; die Hardwareursache ist offen.

Kurvenklasse: `A` steigt am 3.10. und fällt am 4.10.; `B` verhält sich umgekehrt; `C` fällt an beiden Tagen; `D` steigt an beiden; `R` ist annähernd konstant; `U` enthält einen Umkehrpunkt. Die Klassen beschreiben zwei konkrete Passagen, nicht den dauerhaften Sonnenstand einer Fläche.

| Ausgabeplatz | Klasse | Arbeitsname / konkurrierende historische Familie | Stärke |
| --- | --- | --- | --- |
| CH00 | A | Temperatur des sonnenzugewandten Hauptkörpers | W |
| CH01 | R | Kalibrier-/Markerpegel des digitalen Kommutators | S für Marker, W für Referenzfunktion |
| CH02 | B | Strom einer abgewandten Solarflächenhälfte | W |
| CH03 | A | Temperatur des Telemetrie-/Oszillatorgehäuses / Bauteilversuch | W |
| CH04 | A | Spannung der Haupt-Solarflächenkette | W |
| CH05 | A | Einzelstrom einer beleuchteten Solarfläche | W |
| CH06 | A | Sonnenfühler: beleuchteter Quadrant 1 | M |
| CH07 | A | Sonnenfühler: beleuchteter Quadrant 2 | W |
| CH08 | B | Gegenquadrant des Sonnenfühlers zu CH06 | M |
| CH09 | A | Solarflächen-/Reglerausgang A | W |
| CH10 | B | Gegenquadrant des Sonnenfühlers zu CH07 | M |
| CH11 | C | Ladestrom oder Eingang des Akkureglers | W |
| CH12/13 | A/U | Sonnenabhängiger Strom einer Solarfläche, doppelt abgetastet | S für Paar, W für Funktion |
| CH14/15 | A | Thermistor an Solarfläche 1, doppelt abgetastet | S für Paar, W für Funktion |
| CH16/17 | A | Thermistor an Solarfläche 2, doppelt abgetastet | S für Paar, W für Funktion |
| CH18/19 | C | Sonnenabhängiges Energie-/Solarzellensignal, Haupt-Solarstrom als Gerätewette, doppelt abgetastet | S für Paar, M für Sonnenbezug, W für Stromkalibrierung |
| CH20/21 | A | Elektronik-Thermistor / Transistor- oder Solarzellenversuch, doppelt abgetastet | S für Paar, W für Funktion |
| CH22/23 | A | Elektronik-Thermistor / Transistor- oder Solarzellenversuch, doppelt abgetastet | S für Paar, W für Funktion |
| CH24/25 | A | Geregelte Hauptbusspannung, doppelt abgetastet | S für Paar, W für Funktion |
| CH26 | A | DC/DC-Ausgang / Transistor-β-Versuch | W |
| CH27 | A | DC/DC-Ausgang / Transistor-β-Versuch | W |
| CH28 | A | Sensorreferenz / Transistor-Leckstromversuch | W |
| CH29 | A | Wissenschaftsversorgung / Kondensatorversuch | W |
| CH30 | A | Solarflächen-Temperatur / Solarzellendegradation | W |
| CH31 | A | Encoder-Temperatur / Metallsublimationsversuch | W |
| CH32 | B | Strom einer abgewandten Solarflächenhälfte 2 | W |
| CH33 | A | Magnetometer-Elektroniktemperatur / Fluxgate-Achse | W |
| CH34 | A | Außenthermistor / Fluxgate-Achse | W |
| CH35 | A | Strom einer Wissenschaftslast / Fluxgate-Achse | W |
| CH36 | B | Spannung einer beschatteten Solarfläche | W |
| CH37 | B | Strom derselben beschatteten Solarfläche | W |
| CH38 | U/C | Analoger Sonnenwinkel-/Lagegeber | M: dynamischer Verlauf in beiden Passagen; Gleichlauf mit dem unabhängig gewonnenen w13-Gray-Verlauf und passendes historisches Drei-Achsen-Inventar. Gemeinsame Zeit-/Beleuchtungsursache möglich. |
| CH39 | A | Strom eines zweiten großen Solarzweigs | W |
| CH40 | D | Zweite Sonnenwinkelachse oder Schattenkantenindikator | W |
| CH41 | A | RF-Endstufentemperatur / Teilchendetektor-Housekeeping | W |

Die Namen CH03, CH20–35 und CH41 sind bewusst **Arbeitsplatzhalter**. SDO-1600 nennt zusätzlich zu Engineering die Fluxgate-Achsen, Teilchendetektoren und Metallsublimation; Fischell et al. dokumentieren 23 nummerierte Transistor-/Kondensatorversuche. Ihre *Versuchsnummern* werden hier ausdrücklich **nicht** als CH-Nummern gelesen. Aus monotonen Kurven allein lassen sich Temperatur, Strom, Spannung und Bauteiltestsignal nicht sicher trennen. Der Sinn der Zuordnung ist, konkurrierende Familien im Plot gegeneinander zu prüfen; die 35-Quellen-Struktur ist erheblich besser begründet als diese einzelnen Namen.

## Analoger PAM-Kommutator: alle 34 Segmente

| Segment | Geratene Funktion | Stärke / Grund |
| --- | --- | --- |
| 1 | Leer-/Nullposition | S: dauerhaft nahe Null. |
| 2 | Negative Referenz des PAM-Oszillators | S: etwa −330 Hz, kaum Streuung. |
| 3 | Solarflächen-Ausgang A | W: hoher positiver Pegel, über den Überflug fallend. |
| 4 | Leer-/Nullposition | S: nahe Null. |
| 5 | Sonnenfühler, Seite A | M: gegenläufig zu Segment 6; differenzielles Sensorpaar stark gestützt. |
| 6 | Sonnenfühler, Seite B | M: über zwölf Rahmen Summe mit Segment 5 nur 19–37 Hz, Differenz −411 bis +586 Hz. |
| 7 | Hauptbus-/Reglerspannung als Gerätewette | W: auffällige Änderung beim Signalabriss; Empfangs-/Demodulationsverlust als Gegenhypothese. |
| 8 | Gepulster Telltale-/Schalterregister-Kandidat | M für zeitlich strukturierte Pulse, W für Schalterfunktion: in neun Rahmen wiederkehrende Pegel im etwa 20-ms-Raster, aber keine zugeordnete Bitposition oder unabhängige Schaltervalidierung. |
| 9 | Leer-/Nullposition | S: nahe Null. |
| 10 | Solarflächen-Temperatur A | W: mäßig fallender Pegel. |
| 11 | Solarflächen-Temperatur B | W: mäßig steigender Pegel. |
| 12 | Sonnen-/Lagesensor, dritte Achse | W: durchläuft den Nullpunkt. |
| 13 | Elektrische Referenz der Wissenschaftsversorgung | W: annähernd +210 Hz. |
| 14 | Solarflächen-Temperatur C | W: langsamer Anstieg. |
| 15 | Leer-/Nullposition | S: nahe Null. |
| 16 | Paar-Thermistor A1 | W: eng gekoppelt an Segment 17. |
| 17 | Paar-Thermistor A2 | W: eng gekoppelt an Segment 16. |
| 18 | Solarzellen- oder Wandlerstrom A | W: deutlicher Anstieg. |
| 19 | Solarzellen- oder Wandlerstrom B | W: paralleler Anstieg zu Segment 18. |
| 20 | Leer-/Nullposition | S: nahe Null. |
| 21 | Temperatur/Strom der gegenüberliegenden Fläche | W: gegenläufig zur steigenden Familie. |
| 22 | Ruhige Science-Versorgung/Statusleitung, UV-Photometer 1 als Gerätewette | S für nahe konstanten Pegel, W für Gerät: UV-Photonenzahl war historisch digital. |
| 23 | Ruhige Science-Versorgung/Statusleitung, UV-Photometer 2 als Gerätewette | S für nahe konstanten Pegel, W für Gerät: UV-Photonenzahl war historisch digital. |
| 24 | Ruhige Science-Versorgung/Statusleitung, Rb-Magnetometer als Gerätewette | S für nahe konstanten Pegel, W für Gerät: Rb-Feldmessung war historisch ein digitaler Zähler. |
| 25 | Sonnenabhängiger Messwert 1, Panelstrom als Wette | W: fallende Gruppe. |
| 26 | Sonnenabhängiger Messwert 2, Panelstrom als Wette | W: fallende Gruppe. |
| 27 | Sonnenabhängiger Messwert 3, Panelstrom als Wette | W: fallende Gruppe. |
| 28 | Sonnenabhängiger Messwert 4, Panelstrom als Wette | W: fallende Gruppe. |
| 29 | Solarflächen-Summen- oder Reglerstrom | W: fällt mit 25–28. |
| 30 | Positive Referenz des PAM-Oszillators | S: etwa +389 Hz, kaum Streuung. |
| 31 | Zweite feste Referenz-/Versorgungsspannung | M: annähernd +243 Hz. |
| 32 | Temperatur der beschatteten Seite | W: fallende Gruppe. |
| 33 | Offener oder übersteuerter Messkanal 1 | M: analog Rauschen, ADC nahe Vollaussteuerung. |
| 34 | Offener oder übersteuerter Messkanal 2 | M: analog Rauschen, ADC nahe Vollaussteuerung. |

Das ist eine **Arbeitszuordnung für 35/35 vermutete digitale Quellen und 34/34 PAM-Segmente**, aber nur wenige physikalische Namen liegen über der Stufe `W`. Die scheinbar exakten Zuordnungen der vier Solarflächen in PAM 25–28 und der Science-Housekeepingleitungen in 22–24 sind besonders leicht widerlegbar. Weder der digitale noch der analoge Kommutator ist mit dokumentierter Kanaltabelle identifiziert.

## Priorisierte Hinweise und Grenzen

- **PAM 5/6:** In zwölf Rahmen steigt der eine Pegel, während der andere fällt. Ihre Summe schwankt nur zwischen 19 und 37 Hz, die Differenz zwischen −411 und +586 Hz. Ein differenzielles Sensorpaar ist damit deutlich besser gestützt als die konkrete Bezeichnung „Sonnenfühler“.
- **CH38:** Im 3.-Oktober-Durchgang verläuft der Wert etwa 152 → 65 → 76 ADC-Counts, während der unabhängig rekonstruierte w13-Gray-Kandidat durch 55 → 63 → 0 → 6 läuft. Am 4. Oktober fallen CH38 etwa 370 → 294 und der Gray-Kandidat 52 → 38. Ein Sonnen-/Lagebezug ist dadurch ein bevorzugter Kandidat; ein bloßer gemeinsamer Verlauf mit der Überflugzeit bleibt möglich. Winkelachse, Richtung und Skala sind nicht kalibriert.
- **PAM 8:** Neun akzeptierte Rahmen zeigen etwa 66 Messpunkte pro Segment im 5-ms-Raster. Die ersten rund 240 ms enthalten wiederkehrende negative Referenz- und positive oder mittlere Pegel in ungefähr 20-ms-Halbzellen. Das ist ein Hinweis auf serielle Unterkommutierung oder Telltales; ein periodisches Kalibrier-/Testmuster oder ein Demodulationsartefakt ist noch nicht ausgeschlossen. Die sechs möglichen Datenpositionen, ihre Polarität und die physikalischen Schalter sind unbekannt.
- **PAM 22–24:** Die Segmentwerte liegen in den zwölf exportierten Rahmen nahe 20, 13 und 11 Hz. UV-/Rb-Versorgung oder Gerätegesundheit sind konkrete Wetten, die zum vermuteten heutigen Stillstand passen. Die eigentlichen UV- und Rb-Wissenschaftswerte liefen historisch digital; diese drei PAM-Segmente dürfen deshalb nicht als direkt gemessene Photonenzahlen oder Magnetfelder ausgegeben werden. Auch andere Kanalbelegungen bleiben möglich.

## PCM-Wörter, die die PAM-Namen begrenzen

| Feld | Aktueller Schluss | Folge für die Kanalnamen |
| --- | --- | --- |
| w13 b15–b10 | Mindestens sechs bitumgekehrt-Gray-kodierte Schritte, in mehreren Überflügen. SDO-1600 nennt einen digitalen Sonnenlagedetektor. | „Digitaler Sonnenlageindex“ ist ein bevorzugtes Arbeitsmodell; b9 könnte dazugehören. Keine Gradskala. |
| w12 | Mehrere Bits ändern sich, aber weder einfaches Binär- noch Gray-Modell erklärt alle geprüften Durchgänge. | Photometer-/Science-Wort nur eine schwache Wette, **kein** nachgewiesenes 16-Bit-Schalterregister. |
| w3–5, w8, w11, w16 | In den brauchbaren heutigen Rahmen überwiegend Null oder konstant. | Könnten frühere Wissenschaftsregister sein; weder UV noch Rb nach Wortnummer zuweisen. |
| w7/w15 | Digitaler Abgriff des analogen PAM-Hubs. | Keine zusätzlichen unabhängigen Science-Kanäle zählen; prüfen jeden behaupteten PAM-Pegel. |
