# Three further primary/reference PDFs (reviewed 2026-10-26)

These notes concern the user-supplied files. PDF page numbers below are
one-based; printed page numbers are stated separately. A historical source is
not a direct assignment of the received 2026 waveform or PCM words.

## Smith, NASA-CR-74130 / NTRS 19660014433

`/home/jankoch/Downloads/19660014433.pdf`, Andrew M. Smith,
*Stellar Photometry from a Satellite Vehicle*, December 1965, 49 PDF pages.
This directly concerns the two ultraviolet photometers on 1964-83C.

- PDF 4-5, printed 3-4: several heliogoniometers supplied the Sun vector in
  satellite coordinates to within 0.1 degree, together with three orthogonal
  vector magnetometers. Combined attitude and geomagnetic model gave stellar
  aspect accuracy about 0.5-1.5 degrees. The method lacked pointing information
  when the Sun was out of view. This is a system-level precision claim, not a
  conversion formula for a particular PCM word or Gray field.
- PDF 5, printed 4, visually checked: photometer accumulation 1.23 s,
  transmitted every 1.31 s via a PCM channel at **195.3 bit/s**. Each
  photometer had **15 bits** of digital channel capacity. OCR incorrectly reads
  13; the image clearly says 15. 195.3 * 1.31 = 255.843 bits, consistent with
  the known 256-bit frame, subject to rounded published numbers. The reported
  cadence is compatible with one photometer sample per two 0.655-s subframes.
  It does not by itself identify the 15 bit positions or subframe parity.
- PDF 23, printed 22, visually checked: H. P. Lie supervised the electronics
  design/construction; H. D. Black and G. B. Bush solved the attitude problem;
  M. W. Jennings undertook substantial programming for attitude and data
  reduction. These names and project roles give targeted archival search terms.
- PDF 40, figure 1, visually checked: identifies three heliogoniometers,
  magnetometer axes, UV photometer package, access panel, and boom assembly.
  It is a mechanical component diagram, not a channel/encoder map.

Decoder implication: quantitatively compare the measured PCM frame cadence to
1.31 s and identify two independent ~15-bit varying fields or a 15-bit counter
range before labeling a word as photometer. Existing labels of w12/w13 are
unconfirmed. A 15-bit capacity does not guarantee all 15 bits changed in the
sample or explain which photometer is in which frame.

## Danchik, APL Technical Digest 19(1), 1998

`/home/jankoch/Downloads/19-01-Danchik.pdf`, R. J. Danchik,
*An Overview of Transit Development*, 9 PDF pages. Historical overview of the
Transit navigation programme, not a 5E-5 telemetry specification. The 5E series
appears briefly as piggyback environmental/research satellites (PDF 3,
printed 20). Its references (PDF 9, printed 26) cite the *History of the Space
Department at the Applied Physics Laboratory, 1958-1978* (JHU/APL, January
1979), a further archival lead. The acknowledgments refer to unpublished
Kershner manuscripts titled *Transit History* and *The Transit System*; their
relevance to 5E-5 channel assignments is unknown. Avoid treating unrelated
navigation-memory encodings in this article as 5E-5 PCM definitions.

## Potemra, APL Technical Digest 1(3), 1980

`/home/jankoch/Downloads/01-03-Potemra_Studies.pdf`, Thomas A. Potemra,
*Studies of Auroral Field-Aligned Currents with Magsat*, 5 PDF pages. Its
5E-specific content concerns **5E-1 (1963-38C)**, not 5E-5. The later Triad
13-bit ADC is also not a 5E-5 format specification.

- PDF 5, printed 232, note 7, visually checked: the 5E series was designed by
  APL 1962-64 for scientific and engineering measurements of the Transit
  environment; four piggyback launches, three orbital successes. It reports
  **43 journal articles** on 5E findings and independently confirms the
  **27 September 1974 APL symposium** about the 5E programme. It does not
  identify a published proceedings volume.
- References 5 and 6 on the same page are JGR papers by Zmuda et al. on 5E-1
  magnetic observations. Search leads for broader context; no proof of 5E-5
  word/channel assignments.

## Next acquisition targets

1. 5E-5 PCM encoder format, commutator channel allocation, photometer digital
   interface (15 bits each), telltale and heliogoniometer circuits.
2. APL attitude reconstruction and data-reduction reports/software by
   **H. D. Black, G. B. Bush, M. W. Jennings**; electronics records by
   **H. P. Lie**. Search with `1964-83C`, `5E-5`, `UV photometer`, and
   `heliogoniometer`.
3. Programme/abstracts for the September 1974 APL 5E symposium and bibliography
   of the reported 43 articles. The 1979 APL Space Department history may have
   a report list.

No decoder code, bit labels, or photometer field assignment changed.
