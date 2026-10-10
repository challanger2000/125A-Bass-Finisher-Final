# 125A Bass Finisher V1 - User Manual

## Signal flow

`TONE MATCH -> FINISH -> MIX FIT -> OUT`

## Installation

Copy the complete `125A-Bass-Finisher-V1.vst3` folder to
`C:\\Program Files\\Common Files\\VST3\\`, then restart the DAW or rescan VST3 plugins.

## Tone Match

1. LOAD REFERENCE: load the reference.
2. ANALYZE TARGET: play and analyze the target bass.
3. Raise MATCH from 0% to the desired amount.
4. After loading a new reference, analyze the target again.

The `...` menu provides profile load/save plus Clear Target and Clear Reference.

## FINISH

Adaptive bass finishing. 0% is neutral.

Profiles:
- CLEAN - controlled and transparent
- PUNCH - stronger attack and projection
- DENSE - denser and heavier

## MIX FIT

LOW CONTROL:
- Off or 25-90 Hz
- additional adaptive sub control

MASS:
- weight and cleanup
- follows LOW CONTROL so removed sub energy is not simply rebuilt

## OUT

OUTPUT: -12 dB to +12 dB, default 0 dB.

## GUI

Zoom: 100% / 150%.  
Ctrl + left-click resets custom knobs to their actual parameter defaults.

Version 1.0.1 - Windows x64 - VST3


V1.0.1: zero-latency partitioned 4096-tap MATCH FIR CPU optimization. Bass voicing, parameter IDs and project recall remain compatible.
