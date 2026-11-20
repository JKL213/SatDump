# Fremde TRANSIT-Tonaufnahme: unabhängiger PAM-Vergleich

## Herkunft und Grenzen

Untersucht wurde die vom Nutzer bereitgestellte Datei
`/home/jankoch/Downloads/transit-5-b-5-military-navigation-from-1964-hr-0xwa-4x/Transit_5B-5_Military_Navigation_from_1964-[hr0xwa4x].ogg`.
SHA-256: `6bc639d136e73dbd7d26eeb6a9a40df815aa3d8c7f02f72809ea8f546762e369`.
Das lokale Internet-Archive-Metadaten-XML nennt Jordan Patapoff als Uploader, den 19. Dezember 2023 als Archivdatum und den älteren [Clyp-Link](https://clyp.it/hr0xwa4x) als Vorlage. Ein [RTL-SDR.com-Beitrag vom 31. Oktober 2014](https://www.rtl-sdr.com/receiving-dead-satellites-rtl-sdr/comment-page-1/) nennt den Funkamateur „Happysat“ ausdrücklich als Urheber seiner Aufnahme und verlinkt den von Jordan bereitgestellten Stream. Das **Aufnahmedatum**, der genaue Empfangsort, die Empfangsfrequenz und die SDR-Einstellungen sind für genau diese OGG-Datei nicht dokumentiert. „Transit 5B-5“ ist hier eine zeitgenössische Bezeichnung, kein Beleg für die physische C/D-Nutzlastzuordnung.

Die Datei enthält 190,1 Sekunden Ogg Vorbis mit 62 500 Samples/s. Beide nominellen Stereo-Kanäle sind samplegenau identisch: Es ist Mono-Audio und **kein IQ-Mitschnitt**. Der starke Inhalt liegt fast vollständig zwischen etwa 100 und 1 300 Hz. Die eigenen IQ-Messgrößen von 5,45 kHz analog und 9,95/11,25 kHz digital erscheinen darin daher nicht direkt; wahrscheinlich wurde ein schmaler CW-/SSB-Ausschnitt hörbar gemacht. Die konkrete Mischfrequenz und Seitenbandrichtung sind unbekannt. Ein Absolutwert oder Vorzeichen des analogen Telemetriehubs lässt sich daraus nicht ableiten.

## Messung

Das Audio wurde mit `ffmpeg` zu Mono-PCM dekodiert und zur Analyse auf 4 000 Samples/s resampelt. Eine STFT mit 2 048 Samples Fenster und 100 Samples Schritt ergibt eine Zeitabtastung von 25 ms. Pro Schritt wurde die stärkste Spektrallinie zwischen 100 und 1 300 Hz verfolgt. Nach Abzug eines 5-Sekunden-Savitzky–Golay-Trends wurde die Autokorrelation der ersten 60 Sekunden über 10–15 Sekunden Verzögerung berechnet.

Die stärkste Wiederholung liegt bei **12,425 s** (Korrelationskoeffizient **0,761**); beim 25-ms-Rasterpunkt **12,450 s**, praktisch dem selbst gemessenen PAM-Wert von **12,453 s**, beträgt der Koeffizient **0,752**. Für zwei und drei Wiederholungen liegen die Spitzen bei 24,875 s bzw. 37,300 s, entsprechend 12,438 bzw. 12,433 s pro Rahmen. Die Auflösung der Tonspur und die verlustbehaftete Quelle rechtfertigen nur eine Aussage von **rund 12,43–12,45 s**. In vier aufeinanderfolgenden Fenstern wiederholen sich markante Pegelsprünge, während sich die absoluten Tonhöhen verschieben. [Abbildung](transit_external_2014_audio.png).

Damit ist der langsame, gerasterte analoge Rhythmus der eigenen 2026-Aufnahmen durch eine **fremde, schon vor 2023 veröffentlichte Aufnahme** unabhängig gestützt. Es belegt weder die einzelnen PAM-Kanalnamen noch PCM-Syncwörter, Statusbits oder den Zustand eines Auslegers. Der SatDump-Decoder erwartet IQ und kann die OGG-Datei nicht unverändert als Rohdaten einlesen; Vorbis, SSB-Filterung und fehlende 10,6-kHz-PCM-Spur machen sie ungeeignet für einen bitgenauen PCM-Test. Für einen echten Cross-Check wären die ursprüngliche IQ-Datei samt Samplerate, Mittenfrequenz und UTC-Zeit oder wenigstens eine breite lineare FM-/PM-Demodulation nötig.

## Weitere unabhängige Empfangsbeobachtung

Der vom Nutzer nachgereichte [Bericht von Maik Hermenau auf Satellitenwelt](https://www.satellitenwelt.de/transit_5b-5.htm) ist **nicht die Herkunft der Happysat-/Jordan-OGG-Datei**. Hermenau schildert eigene SSB-Empfangsversuche ab Anfang 2002 auf 136,650 MHz und berichtet Signale in etwa 5,4 und 10 kHz Abstand vom Träger. Er beschreibt den analogen Zweig als PAM mit einem Muster, das sich ungefähr alle 12,5 s wiederholt, und den zweiten Zweig als PCM. Die Seite enthält getrennte Tonbeispiele für Träger, analogen und digitalen Seitenbandbereich sowie einen Ausfall beim Eintritt in den Erdschatten; der angegebene letzte Seitenstand ist 17. März 2013. Das ist eine zweite, ältere Beobachtung, die unsere Seitenbandpositionen und Rahmenperiode qualitativ stützt. Die Angabe „12,5 s“ ist gerundet, keine Präzisionsmessung.

Die dortigen Erklärungen zu Batterie, Lagekontrolle, Leistung, Kommandierbarkeit, RTG und zur physischen 5B-5/5E-5-Identität sind Hypothesen oder historische Zuschreibungen. Aus den beschriebenen SSB-Spektren allein folgt keine dieser technischen Zuordnungen und auch keine Statusbit-Tabelle. Die Tonbeispiele können für Form- und Ausfallvergleiche dienen; ein belastbarer Bitvergleich braucht wiederum breitbandige Rohdaten und Aufnahme-Metadaten.
