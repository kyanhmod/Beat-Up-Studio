# Audition Old: chart phase and HUD size

Reference: `(Nhi)Audition_OldVTC.exe`, SHA256
`398b1e1c486b1ae673f49a39734deaa32bcf3e1840a5f4b0cc5dc4bd1976d981`.
Reproduce disassembly with `python tools/inspect_old_gameplay.py`.

## Verified clock behavior

The executable has a per-song offset, rather than a universal hardcoded 0.7.
At 0x963F51 the song record's BPM (+0x18C) and offset (+0x190) are copied
into the active song's +0x38/+0x3C. Custom-song records also supply an offset.
At 0x964664 the sixteenth-note duration is calculated as 15000/BPM ms.
The smooth clock (0x9BC160, tail 0x9BC22C) returns:

    chartClockMs = elapsedAudioMs + (1 - songOffset) * tickMs

The integer clock at 0x9BC260 uses the same phase before dividing by tickMs.
The gameplay update passes this clock to its renderer at 0xA13002.
This establishes the unit and phase convention; it does not establish a
single correct offset for every chart/audio pair or identify every catalog column.

Cdiu's cma3.bas initializes OffsetSet to 0.7. cma2.bas converts that setting
to milliseconds using CInt(15000/BPM*OffsetSet). Thus 0.7 means 0.7 of a
sixteenth-note tick: 70ms at 150 BPM. Its judgement clock also has an index
origin term, and its visual clock does not apply the same offset. Copying
70ms directly into this player's offset_ms would therefore not reproduce
the original timing convention.

## Explicit opt-in setting

The simulator's native Location rows use time = Location*tickMs. An optional
`audition_offset=0.7` in a chart's `.song.ini` applies the inverted native
clock phase to those timestamps:

    eventMs = (Location + audition_offset - 1) * tickMs + offset_ms

At 150 BPM this is a -30ms shift relative to bare Location timing, not +70ms.
This conversion follows the verified clock with the simulator's existing row
origin; parity of all native chart-loader variants remains to be checked.
It is not a Cdiu-list importer. Existing songs receive no shift unless this
setting is present. Absolute TIME charts only use offset_ms. BPM changes
recalculate from original ticks, and restart does not accumulate offsets.

## HUD correction and input preservation

Native combo scale at 0xA0C05E is max(0.8, 1.4 - elapsed/tickMs).
The old simulator used the beat pulse (1.0–1.15), so settled digits were
at least 25% too large. The counter now uses the native hit pulse and 0.8
settled scale. Native judgement scale remains max(1, 1.8 - elapsed/tickMs).
The source digit size, advance and centers remain unchanged.

Numpad 0 and 5 now trigger Space in both key schemes. Scan-code normalization
handles NumLock-off Insert/Clear while preserving extended navigation keys.
This is the sole change to the accepted main.cpp input pipeline; event
timestamps, repeat suppression, audio clock and BeatUpRules are retained.

Remaining native work: chance/finish transitions, score/gauge multipliers and
catalog formats. Editor migration order is in CDIU_MIGRATION.md.

Validation: core tests cover native offset parsing, phase arithmetic, BPM scaling
and NumLock-off scan codes. The renderer/game tests exercise all three Space
keys, repeat suppression, offset judgement, repeated restart and BPM retiming,
alongside the existing manual feel and combo gauge regressions. A rendered
100ms-after-hit capture was inspected for combo/judgement size. AudioEngine.cpp
retains its zero-origin interpolation behavior; later changes add independent
gains and explicit pause/seek clock origins for transport. See
NATIVE_SEQUENCE_AND_TRANSPORT.md. BeatUpRules.h retains its accepted-baseline hash.
