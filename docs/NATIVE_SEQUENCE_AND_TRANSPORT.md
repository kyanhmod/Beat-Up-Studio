# Audition Old sequence and explicit playback workflow

Reference binary SHA256:
`398b1e1c486b1ae673f49a39734deaa32bcf3e1840a5f4b0cc5dc4bd1976d981`.
Run `python tools/inspect_old_gameplay.py` to reproduce the native instruction
blocks. Cdiu is the workflow reference, not the authority for the rules below.

## Native combo loss

Audition Old stores both combo (+0x6508) and gauge level (+0x650C). Good hits
recompute the level from the six thresholds at 0xE93158: 10,20,40,60,80,100
(0xA0D83D..0xA0D895). Miss at 0xA0D9EB..0xA0DA68 does this:

- Level 0: combo becomes zero.
- Level 6: combo and gauge become zero.
- Otherwise decrement level, then set combo to thresholds[newLevel].

Examples: 99 -> 80 -> 60 -> 40 -> 20 -> 10 -> 0 under consecutive misses.
At exactly 10, the first miss retains the number 10 but extinguishes its first
gauge letter; a second miss clears it. This requires persistent level state,
not recomputing level from combo on every miss. At 100 the full-gauge branch
clears immediately. This differs from Cdiu cma1.FixCombo's decimal subtraction.
Bad retains combo as before. Native judgement windows and note travel are unchanged.

## Native READY / START / count

At 0x8E6320..0x8E6369, current integer tick is compared to the first note tick:
before first-32 is R (getready), first-32..first-16 is S (start), and the final
16 ticks use C. Count is trunc(4 - 4*progress), progress = elapsed ticks / 16;
only values 3,2,1 draw. The integer tick clock matters at count boundaries.
The mode dispatch at 0x8E615B/0x8E6166 confirms Beat-Up uses this branch.
Presentation adjustment requested after playtesting: suppress READY before
first-48 ticks, show READY during first-48..first-32 and START during
first-32..first-16 (four beats each). The final native countdown is unchanged.
This is a presentation preference, not a newly discovered native timing rule;
the music clock, chart offset and judgement timing are unchanged.

The Beat-Up draw branch at 0x8E6CC8..0x8E6DD0 uses ready/start top-left 382,382
and count top-left 369,367. Original DDS sizes are used: ready/start 256x64,
count 2/3 64x64, count 1 32x64. `tools/prepare_intro_assets.py` copies the
original Korean-named DDS files, with hashes in intro_assets.json.

The SOUND directory supplies BEAT, SPACE, GREAT, MISS, READY and START OGG files.
They are decoded once, converted to the existing mono 44.1kHz effect voice
format, and played through the preallocated voice pool and effect-volume gain.
`bin/SOUND` has priority. Build packaging fills missing files only, preserving
user-supplied sounds. The old synthetic BEAT/MISS remain only as a missing-file
fallback, alongside the metronome; the shipped six effects come from SOUND.

## Session and transport

`bin/BeatUpStudio.exe` launches without a selected song and without playback.
File opens OGG/WAV and SLK; the embedded settings panel exposes BPM and
native tick offset, and provides Play/Space, Pause/F9, Restart, seeking, editor
and inline music/effect volume sliders. F10 restores the toolbar if hidden. The Play button becomes Pause while running
and Resume while paused; F9 or the Pause key also toggles playback. Loading or choosing a position
does not start playback. Music metadata can prefill values; user fields are
validated and applied when Play is pressed. Playback requires music; a new song starts with an empty chart.
Opening the editor also commits the entered timing fields so it inherits session
settings even before first playback. A manually selected music file is retained
across subsequent SLK loads; the toolbar names both inputs.

Seek resubmits the already-decoded PCM from a sample-frame PlayBegin and anchors
the clock at that frame. It resets pending note/feedback state and ignores earlier events, while retaining
combo, gauge level, score, maximum combo and judgement counts. Restart clears
these statistics; seeking forward or backward does not.
Paused seek stays silent; dragging during playback pauses and resumes after
release. Pause holds the source sample clock; Resume retains its origin. Restart
from zero uses the original PlayMusic path, with its existing interpolation and
timestamped key handling. Idle/paused frames never advance judgement or combo.

F6 pauses manual play and switches the same window to the editor. F5 previews its current in-memory snapshot
from the cursor with one beat of lead-in, keeping the editor visible.
Preview uses autoplay, shows a moving playhead and follows the timeline. It has
Pause/Resume/Restart/Transport buttons. Closing the editor pauses preview and
restores the previous manual/autoplay setting at that position. Editor changes
are included in the next F5 snapshot; there is no claim of live unsaved note edits
being reflected in an already-running playback snapshot.

## Validation

Core tests exercise native gauge state, intro tick boundaries, all six SOUND
decodes, pause/resume, silent seek, backward seek and zero-origin restart.
Renderer tests cover real 99-combo miss -> 80 -> 60 and the gauge lights, capture
READY/START/3/2/1, and exercise the actual transport window with real music.
Editor tests verify cursor-preview timing after pending BPM/offset edits,
remaining open during preview, playhead rendering and transport command routing.

Final validation status: the app builds, core tests pass, and the updated editor
tests (including session-timing inheritance) pass. An earlier complete transport
and renderer run passed. The final renderer-test executable was denied execution
by Windows and was subsequently absent from disk; the cause is unconfirmed.
The request to rerun outside the sandbox was declined, and no such run occurred.
Do not report that final renderer run as passing.

Background menu: Black (default, RGB 0 including letterbox), or original bg.dds
at 1024x768. F5 toggles these in the player; editor F5 remains preview.

The editor captures mouse drags to select an inclusive tick range across all
lanes. A plain click toggles one note on mouse release; a drag never toggles
the first note. Shift extends the existing selection. Dragging beyond the
horizontal grid edges scrolls the selection. Capture loss cancels the gesture.

See [Cdiu session workflow](CDIU_SESSION_WORKFLOW.md) for the music-first
setup, explicit mode menus and zero-note editor/playback flow.

Current UI/layout and corrected native voice phase mapping are documented in
[UI_REFACTOR_AND_LAYOUT.md](UI_REFACTOR_AND_LAYOUT.md). 4:3 is the default;
wide mode is explicitly optional.
