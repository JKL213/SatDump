# TRANSIT: bewusst spekulatives Arbeitsmodell

Stand 27. Oktober 2026. **Dies ist ein Guess-Modell, keine dekodierte Kanalbelegung.** Es soll plausible Namen zum Vergleich liefern; Rohwerte, Bitpositionen und das verifizierte Exportformat bleiben maßgeblich. Ausgangspunkte sind der [Forschungsstand](TRANSIT_5E5_FORSCHUNGSSTAND_2026-10-27.md), der [Quellenindex mit heutigem Abgleich](TRANSIT_5E5_QUELLENINDEX_2026-10-27.md), die [Statusprüfung](../../../build/transit_status_audit/report.md), [Kenny 2010](https://emitters.space/1964-83D.pdf), [Hermenau](https://www.satellitenwelt.de/transit_5b-5.htm) und [Xerbo](https://xerbo.net/posts/investigating-transit-pt1/).

## Systemmodell, wenn wir eine Geschichte festlegen müssen

1. **Der Sender auf dem Bahnkörper 1964-083D/NORAD 965 führt die 5E-5-artige Wissenschaftstelemetrie.** Die katalogisierte C/D-Nutzlastzuordnung ist dann entweder vertauscht oder die technische Nutzlast-/Sendergrenze anders als die Objektbezeichnung. Das erklärt gleichzeitig Dopplerfit und wissenschaftlichen PAM/PCM-Aufbau. Es ist eine Identitätshypothese, keine Neubestimmung des Satelliten.
2. **Ein gemeinsamer Taktteiler wurde nach 1973 auf doppelte Geschwindigkeit geschaltet oder ist ausgefallen.** Der von Kenny aus einer 1973-Aufnahme abgeleitete 24,9-s-Rahmen und historische 195,3 bit/s werden zu heutigen 12,453 s und 390,594 bit/s; 19 PCM-Subframes bleiben pro PAM-Rahmen. Ein irrtümlich als Doppelrahmen gelesener 1973-Mitschnitt bleibt eine ernsthafte Alternative. Die 2014 verfügbare Fremdaufnahme zeigt bereits ungefähr 12,43–12,45 s.
3. **w12 ist wahrscheinlich kein Wort aus 16 unabhängigen Statusschaltern.** Es wechselt in mehreren Bits und folgt keinem einfachen Binär-/Gray-Feld über die geprüften Passagen. Mein bevorzugter Platzhalter ist ein gepacktes Wissenschafts-/Engineeringwort oder ein beschädigt/schlecht ausgerichteter Sensorstrom; „UV-Photometerwert“ wäre der mutigere konkrete Name. Die historischen zwei 15-Bit-UV-Photometer liefern dafür ein mögliches Instrument, aber keine Wortadresse oder nachgewiesene heutige 1,31-s-Kadenz.
4. **PAM-Segment 8 ist ein bevorzugter Kandidat für seriellen Schalterstatus.** Seine reproduzierbaren kurzen Pulse liefern ein Indiz für Unterkommutierung, aber noch keinen Schalternachweis. w13 enthält im oberen Teil eher eine abgestufte Lage-/Sonnenzahl. Die bisherige UI-Bezeichnung „Statusflags“ für alle w12-/w13-Bits sollte als Rohbit-Anzeige gelesen werden, nicht als 28 belegte Schalter.

## Vorläufiges PCM-Wörterbuch

| Wort, 1-basiert | Beste Arbeitsbezeichnung | Stärke des Schlusses |
| --- | --- | --- |
| w1 | Sync A/B `E240`/`E25F` | Gemessen. |
| w2, w6, w10, w14 | Vier 12-Bit-Abtastungen eines periodischen digitalen Kommutatorstroms je Subframe | Gemessen; konkrete Instrumente unbekannt. |
| w3, w4, w5, w8, w11, w16 | Nicht belegte, abgeschaltete oder derzeit konstante Quellen | Null-/Konstantenverhalten beobachtet; physikalische Ursache geraten. |
| w7, w15 | Digitale Abtastungen des analogen PAM-Hubs | Durch PCM/PAM-Korrelation stark gestützt. |
| w9 | 16-Bit-Subframe-/Hauptrahmenzähler | Durch Folgebeziehungen stark gestützt. |
| w12 | Gepacktes Wissenschafts-/Engineeringwort, **als konkrete Wette: UV-Photometer/Instrumentenstatus** | Sehr spekulativ; weder Format noch physikalische Adresse geklärt. |
| w13 b15…b10 | 6-Bit, bitumgekehrt-Gray-kodierter Sonnen-/Lageindex | Codefolge stark, Sensorname geraten. |
| w13 b9 | Entweder siebtes Gray-Bit **oder** Bereichs-/Vorzeichenbit der Sonnenmessung | b9 ist im 3.-Oktober-Durchgang fast stets 0, in zwei 4.-Oktober-Passagen fast/stets 1. Das ist kein Beweis für einen unabhängigen Schalter. |
| w13 b8…b0 | Residualdaten, Gültigkeits-/Modeinformation oder Rauschen | Niedrige Bits zeigen viele Einzel-Frame-Ausschläge; kein verantwortbarer physikalischer Einzelbitname. |

Für die w13-Deutung ist **6-Bit-Gray plus b9 als Bereichsbit** mein bevorzugtes Modell: Es erhält die beobachteten glatten 6-Bit-Folgen und erklärt den passweisen b9-Wechsel. SDO-1600 nennt ausdrücklich ein digitales Sonnenlageexperiment, was die *Funktionsfamilie* stützt, aber weder genau dieses Wort noch die Skala belegt. Ein echtes 7-Bit-Gray-Feld ist fast ebenso plausibel und muss mit mehr Überflügen getestet werden. Eine 8-Bit-Deutung erzeugt in den geprüften H-Frames deutlich mehr einzelne Ausreißer.

## Vorläufiges PAM-Wörterbuch

| Segmente, ohne Sync | Arbeitsname | Grundlage |
| --- | --- | --- |
| 1, 4, 9, 15, 20 | Leer-/Nullpositionen | Pegel nahe Null; eventuell absichtlich unbestückt. |
| 2, 30 | Untere/obere elektrische Kalibrierreferenz | Fast konstante −330/+389 Hz. |
| 3, 10, 21, 25–29, 32 | Sonnenseitig belastete Zell-/Temperaturgruppe | Gemeinsame fallende Verläufe; einzelne Funktionen nicht unterscheidbar. |
| 5, 6 | Differenzielles Sonnenlage-Sensorpaar | Gegenläufig bei ungefähr konstanter Summe. |
| 7 | Hauptbus-/Reglerspannung | Abrissreaktion; noch keine technische Einheit. |
| 8 | Serieller Telltale-/Statuskanal als Kandidat | Wiederholte Drei-Pegel-Impulse im etwa 20-ms-Raster; Funktion und Schalterpositionen ungeprüft. |
| 11, 12, 14, 16–19 | Gegenüberliegende Zell-/Temperaturgruppe | Gemeinsame steigende Verläufe. |
| 13, 31 | Referenz, festes Experiment oder stabile Spannung | Annähernd konstant. |
| 22–24 | Ruhende Wissenschafts-Housekeeping-Eingänge | Pegel nahe Null; passt zur Vermutung heute inaktiver UV- und Magnetometerexperimente, ohne deren Kanaladressen zu beweisen. Die eigentlichen UV- und Rb-Messwerte waren historisch digital. |
| 33, 34 | Offener, übersteuerter oder defekter Eingang | Analog rauschartig, digital nahe Vollaussteuerung. |

Für den **digitalen** 42-Werte-Strom würde ich CH1 als Marker/Kalibrierwert, CH18/19 als stark sonnenabhängige Leistungs- oder Solarflächenströme und CH38 als groben Sonnen-/Orientierungskanal etikettieren. CH12–25 bilden in zwei Aufnahmen sieben nahezu identische Nachbarpaare; nach Abzug dieser Wiederholungen bleiben 35 mögliche Quellen. Die übrigen Werte lassen sich nach „Power“, „Thermal“, „Attitude“ und „Science“ gruppieren, doch ihre Einzelbelegung bleibt schwach begründet. Die [vollständige geratene Kanalbelegung](TRANSIT_5E5_KANALRATEN.md) gibt deshalb für jeden Ausgabeplatz einen ausdrücklich spekulativen Arbeitsnamen. Die Übereinstimmung mit der historischen Zahl 35 beweist weder den Mechanismus der Doppelwerte noch die Identität des Kommutators.

## Telltale-Kandidat aus PAM-Segment 8

Nach gemeinsamer Flankenausrichtung von neun akzeptierten PAM-Rahmen erscheinen in den ersten etwa 240 ms sechs negative Referenz-Halbzellen in ungefähr 40-ms-Abständen. In den darauf folgenden Halbzellen gibt es ein wiederkehrendes Muster aus positiven und nahezu neutralen Pegeln, grob **`+ 0 0 0 + +`**. Das könnte ein sechsstelliger Telltale-Code mit je Referenz- und Datenhälfte sein; der restliche Segmentteil könnte rahmeninterne Synchronisation oder Rücklauf sein. Die Phasenlage schwankt um mehrere 5-ms-Abtastungen, und die Polarität `+ = 1` ist eine **freie Annahme**.

Als zu prüfendes **Funktionsvokabular**, ohne belegte Reihenfolge, kommen aus Kennys Vergleich und den 5E-5-Berichten Trennung, Solarflächenentfaltung, Entdrallung, Auslegerfreigabe/-bewegung/-Endschalter und Magnet-/Lagekontrolle in Frage. Wenn man trotz fehlendem Kanalplan einen heutigen Zustand raten will: Trennung und Solarversorgung aktiv, Ausleger-Endschalter möglicherweise nie erreicht, Magnetometerexperiment und aktive Lagekontrolle wahrscheinlich inaktiv. Ein eingefrorener historischer Telltale-Wert könnte dabei trotz fehlender aktueller Funktion unverändert bleiben. **Keiner dieser Zustände ist aus `+ 0 0 0 + +` direkt dekodiert.**

## Drei schnelle Widerlegungen

1. Ein sauber digitalisiertes 1973-Band zeigt ebenfalls 12,45 s: Dann fällt die Verdopplungs-/Teilerhypothese.
2. w13 b9 wechselt innerhalb eines störungsarmen Überflugs unabhängig von den Gray-Schritten: Dann ist „siebtes Gray-Bit“ unwahrscheinlich; Bereichsbit, Modus oder anderer Schalter werden stärker.
3. PAM-Segment-8-Pulse wechseln nicht reproduzierbar in festen Zeitpositionen zwischen Passagen: Dann ist das sechsstellige Telltale-Modell eher Demodulator-/Rahmenartefakt als ein Statusregister.
