# Gameplay reconstruction: OldVTC

Reference binary: `(Nhi)Audition_OldVTC.exe`, SHA-256
`398b1e1c486b1ae673f49a39734deaa32bcf3e1840a5f4b0cc5dc4bd1976d981`.
Reproduce the trace with `python tools/inspect_old_gameplay.py`.

## Motion and judgement linked by pixel distance

0x964664..0x96468F derives sixteenth tick duration from `(240000 / BPM) / 16`.
0xA125E0..0xA12613 sets direction speed to `324 / (8 * tickMs)` and Space
speed to `134 / (8 * tickMs)`. At 150 BPM these are **0.405 px/ms** and
**0.1675 px/ms**, respectively. Previously both were guessed and BPM-independent.

0xA13221..0xA132E2 calculates signed pixel distance, then calls 0xA0BA20.
The 6-lane input branch uses centers **319 / 705**; the other branch uses
404 / 620. Early presses beyond 55 pixels are ignored. Tables at 0xE93130:

| Judgement | Absolute distance, direction |
| --- | --- |
| Perfect | 0..5 pixels |
| Great | 6..15 |
| Cool | 16..27 |
| Bad | 28..40 |
| Miss | 41..55; overdue input also returns Miss |

These are not fixed millisecond windows. At 150 BPM, 5 pixels is about 12.35 ms
before integer pixel rounding; the previous simulator used 35 ms. Native
0xB0A570 truncates float-to-integer using CVTTSD2SI. Left/right rounding can
differ by one pixel because the code quantizes screen position.

Space is separate: 0xA0BA70 uses **3 / 8 / 20 / 32 / 45 pixels**, immediately
returning Miss below **-8 pixels**. 0xA0EAC0 ignores early inputs beyond 55;
the judge also returns no result for positive distances 46..55.

The input path 0xA131D0..0xA1320C chooses the first pending note in chart order
matching the direction. It does not search for the nearest timestamp.

## Visual changes

- Center-coordinate arrows use the same integer position model as judging.
- Space notes keep their native 64x32 dimensions, meet at (512,656), and
  appear within the native 134-pixel approach distance.
- The Space bar origin is (357,623), traced at 0xA10D42.
- Judgement uses the texture's real aspect ratio and native center (513,194).
  Combo digits use square textures and 32-pixel advance around y=242.
- Hit spark uses the full 256x256 texture with a short shrinking pulse;
  receptor feedback and waves use the same lane centers as the arrows.
- The screenshot background is optional (F5), initially disabled because
  its baked MISS/GET READY/watermark obscure actual simulator feedback.
- song_info composition is preserved.

## Input and audio changes

Queued Windows key events use their message timestamps rather than dispatch
time. NumLock-off keypad scan codes map to the same directions. Key repeat
cannot consume another note; focus loss clears held keys. Autoplay directly
judges due notes at zero error, independently of the live audio clock.

The effect voice pool is created at initialization, avoiding source-voice
allocation for every hit. Effects still use synthesized audio, not extracted
native hit samples. No end-to-end latency number is claimed.

## Limits of this pass

The supplied skin and native branch offsets are not identical in all places;
the layout retains the supplied skin's six-lane receptor arrangement. Only one
layout is implemented. Native chance toggling, finish sequences, full combo/gauge
scoring, and note miss/fade lifecycle are not reconstructed completely.

Direction notes retire input at 27 pixels past the receptor and freeze/fade
over the next 100 pixels (0xA0D394..0xA0D45E and the mirrored right branch).
The simulator reconstructs that lifecycle. Native frame ordering around the
boundary and its stored previous-frame position can differ by a frame. Native
arrow animation is distance based; its phase/timing and blend details still
need video comparison. Judgement/score values are base values from native
tables, without the full chance/gauge multipliers.

The automated checks verify code behavior and pixel alignment, not subjective
equivalence to the game. Audio-device latency, render queues and native sound
samples still affect perceived feel.

## Video reference follow-up

User reference: https://www.youtube.com/watch?v=3OxyU7q03Bk
(`[17au Audition] Beat Up - Audition - Party Party (Lv.5)`, 130 BPM).
Inspected in the browser, including paused frames around 15.66..15.87 seconds.
The available video stream is 480x360; this is a qualitative motion/feedback
reference, not a source of precise physical input latency measurements.

Corrections after this comparison and another native-code pass:

- End caps now draw after notes and feedback. Native caller order at
  0xA1193D -> 0xA11A1C renders arrow objects, then invokes the end-cap renderer
  0xA0CD40. Drawing caps first let arrows fly over the decorative masks.
- Hit arrows no longer disappear instantly. At 0xA0D0D0..0xA0D1B7 native code
  retains the hit position and fades an arrow over two sixteenth ticks.
  At 150 BPM that is 200 ms; the prior spark lasted only 50 ms.
- The shrinking glow is limited to Perfect/Great, following the native
  comparison with judgement index 2 at 0xA0D1BA. Cool/Bad retain the fading
  arrow. Held keys no longer leave a continuous starburst at the receptor.
- Space uses the supplied Space-hit texture rather than a squeezed blue
  directional starburst. Its exact envelope still needs a dedicated trace.
- XAudio sample count is now interpolated with QPC between processing passes,
  bounded to 20 ms beyond the last observed sample position and clamped to song
  duration. This is a modern transport implementation decision, not extracted
  native code, and does not measure or cancel output-device latency.

Regression captures include `spawn_mask.bmp`, `hit_100ms.bmp` and
`hit_250ms.bmp` in `tests/runtime`. Test cases cover a released key and the
continuing hit animation. The hit effect remains at 100 ms and is gone by
250 ms for a 150 BPM fixture. Gameplay timing thresholds were not adjusted
based on subjective guesses from this low-resolution video.
