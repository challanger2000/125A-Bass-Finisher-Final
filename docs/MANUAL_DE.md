# 125A Bass Finisher V1 - Bedienungsanleitung

## Signalfluss

`TONE MATCH -> FINISH -> MIX FIT -> OUT`

## Installation

Den kompletten Ordner `125A-Bass-Finisher-V1.vst3` nach
`C:\\Program Files\\Common Files\\VST3\\` kopieren und die DAW neu starten bzw. VST3 neu scannen.

## Tone Match

1. LOAD REFERENCE: Referenz laden.
2. ANALYZE TARGET: Ziel-Bass abspielen und analysieren.
3. MATCH von 0% aus auf den gewuenschten Anteil einstellen.
4. Nach einer neuen Referenz das Target erneut analysieren.

Das Menue `...` bietet Profil laden/speichern sowie Clear Target und Clear Reference.

## FINISH

Adaptive Bass-Nachbearbeitung. 0% ist neutral.

Profile:
- CLEAN - kontrolliert und transparent
- PUNCH - staerkerer Attack-/Durchsetzungsfokus
- DENSE - dichter und kraeftiger

## MIX FIT

LOW CONTROL:
- Off oder 25-90 Hz
- zusaetzliche adaptive Sub-Kontrolle

MASS:
- Gewicht und Cleanup
- folgt LOW CONTROL, damit entfernte Sub-Energie nicht einfach wieder aufgebaut wird

## OUT

OUTPUT: -12 dB bis +12 dB, Default 0 dB.

## GUI

Zoom: 100% / 150%.  
Ctrl + Linksklick setzt Custom-Knobs auf ihren echten Parameter-Default zurueck.

Version 1.0.1 - Windows x64 - VST3

V1.0.1: Weniger CPU-Last durch latenzfreie, partitionierte 4096-Tap-MATCH-Faltung. Bass-Klangcharakter, alle Regler, Presets und Projekte bleiben kompatibel.
