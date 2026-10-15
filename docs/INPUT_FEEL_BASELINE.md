# Approved manual-input baseline

On 2026-10-15 the user selected the current manual-play feel as a strength to
retain while replacing Cdiu. Preserve the existing input/transport implementation
when adding editor and visual features. Cdiu is a reference for feature behavior,
not a reason to replace this input path with its legacy engine.

`baseline/approved_input.json` records the exact approved audio, timing rules
and Windows-event source hashes. `baseline/BeatUpSimulator_approved_input.exe`
is a copy of the accepted build, saved before adding the combo gauge. To run
that archived build, copy it beside the packaged assets in `bin` under a different
name; its resource paths are relative to its executable directory.

Preserve these practical properties:

- timestamp queued key events rather than use the previous rendered frame;
- stable Numpad identity with NumLock off and no repeat-key note consumption;
- current sample/QPC transport interpolation and restart reset;
- pixel judgement and note travel model, including distinct Space rules;
- current manual press/release behavior and fading hit feedback.

The combo gauge change adds visual state after gameplay updates and uses the
existing combo count. It does not change input timestamps, travel speed,
judgement windows, hit sound scheduling or score calculation. AudioEngine,
BeatUpRules and main.cpp hashes are unchanged from the accepted build.

Run `tests/run.bat` before/after adding editor functions. In particular, retain
the manual-alignment, first-pending-note, key-repeat, held-key-release, autoplay,
audio clock and restart checks. Subjective comparison still needs real manual
play; passing tests alone is not evidence that feel is identical.
# Later output/control additions

The Finish Move sprite and horizontal editor add presentation and authoring
features. AudioEngine now exposes independent music/effect gains and reapplies
music gain after restart. Its sample-interpolated clock, buffered sample origin,
preallocated effect voices and timestamped input behavior are retained; existing
hashes below identify the historical accepted baseline rather than the new
gain-enabled AudioEngine source. BeatUpRules.h is unchanged. The regression suite
checks live volume changes without resetting the clock and unchanged manual
judgement/motion behavior.

