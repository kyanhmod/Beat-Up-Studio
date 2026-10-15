# Current overlay and background correction

The 560x64 transport overlays the upper-right corner of the full client canvas.
Showing/hiding it never reserves a strip, resizes the renderer or changes the
4:3 gameplay scale. Editor controls share the top area without a separate header.
The supplied bg.dds is RGB24, now expanded to BGRA8 by the DDS loader (including
row-pitch validation). Previously it was silently unsupported. Session regression
images exercise the actual background with the overlay shown and hidden.

Current compact transport: 560x64 logical pixels, top right. Three circular
icon buttons (Play/Pause, Restart, Settings), Space/beat navigation, seek,
music/effect gains and elapsed/total time only. Tooltips explain each icon.
Mode selection stays in the Mode menu; the transport has no duplicate mode
buttons, filename, note count or playback-state sentence. The main title is
fixed at Beat-Up Studio with no periodic debug/statistics updates.

# Current display behavior

Only the original 4:3 canvas is available. Widescreen mode, its View menu and
runtime extended-mask path have been removed at the user's request. Original
supplied DDS files are retained. The viewport keeps aspect ratio at any window
size; unused areas and the gap beside the compact toggle paint black.
Key assist defaults to off; F3 remains a reversible on/off shortcut.

The native layout research below is retained as historical reference, not as
an available product feature.

# Studio shell, 4:3 default, native sequence and Chance

## Workspace

File actions stay in the menu. One compact 640x82 logical-pixel transport sits
at the top right: Play/Restart/Settings, horizontal gains, Game/Auto/Editor,
seek controls and one short status line. There is no separate large header or
full-width bottom transport. Resizing the app does not stretch this cluster.
F10 hides/shows it; F6 switches player/editor. Settings uses the same height so
committing BPM/offset cannot resize the player area.

The embedded editor never changes the owner's window size. Its rows adapt to
the existing client area; switching back keeps both the outer window rectangle
and the player rectangle identical. Editing actions stay in the menu plus one
short row of Undo/Redo/Copy/Cut/Paste/Preview controls. Native DDS arrow art and
the shared dark theme are retained. Play returns keyboard focus to gameplay.

4:3 is the default at every window size, including fullscreen. Resize preserves
sprite aspect and the judgement coordinates, adding black margins as required.
View offers an explicit optional widescreen mode. Manual judgement rules and
message timestamps are unchanged (BeatUpRules.h hash retained).

## Optional widescreen evidence

Audition_newPH.exe SHA256:
33a5abf71795073c6dc066fb6b7943bfe280fa9069e1ac8269f16b563b6aedc4

0x4584C3..0x458508 draws the two outer masks at (-171,419) and (897,419).
Names at 0xBCFF48/0xBCFF54 are the Korean note-line L_3/R_3. The supplied
299x256 DDS files are copied without conversion to track_wide_l/r.dds.
Run tools/inspect_new_layout.py to reproduce these instructions.

The centre gameplay coordinate system stays 1024x768. Wide mode adds side
margins up to 171 logical pixels and extends only the outer masks. It does not
stretch arrows, receptors or judgement distances. Wider displays retain black
side margins beyond the native wide canvas. bg.dds uses a centred cover crop,
preserving its aspect instead of making characters wider.

## READY / START

Removed the prior two-beat presentation override. READY occupies the four beats
before START; START retains the native sixteen sixteenth-note ticks (four beats),
then the native sixteen-tick countdown runs. READY is bounded to avoid showing
it through an arbitrarily long song intro. This READY onset is a UI choice;
the START/count transitions are native.

The previous audio mapping was one phase early. OldVTC 0x8E638E..0x8E63EF plays
voice_r/ready in the S phase, not R. 0x8E64F0..0x8E6584 plays the go/start voice
in C. Cdiu cma4.CheckStart/RenderReady agrees: READY.ogg at Number=1 (START
image), START.ogg at Number=2 (countdown). Audio now follows those phase gates.
Music, note timestamps and chart offset are unchanged.

## Del Chance

The original press chance key.dds is A4R4G4B4 16-bit, previously rejected by the
loader. DDSLoader now expands masked 16-bit RGB/alpha to BGRA8 on load, retaining
the original asset. It validates the source pitch/length before conversion.

OldVTC 0xA11AE8..0xA11D1F requires score >5000 and a pending Finish; prompt
window is finish measure minus 12 through minus 10. These are measures, not
sixteenth-note ticks: 0xA11279..0xA1127F compares note tick >>4 to finish-10.
The native prompt slides from x=-210 to zero over 500ms, at y=395. Delete
activates it and dismisses the prompt. Space and Finish markers are excluded
from pink-note generation; activation ends at Finish or resets on seek/restart.
Pink notes are BAD if pressed (native note flag path at 0xA132C0..0xA132E2,
0xA0BA5A..0xA0BA69) and do not become normal misses when left alone.

The native random<50 generation is represented by a repeatable roughly half
selection for reproducible editing previews. Exact native RNG and its separate
Chance score bonus are not claimed reproduced in this change.

## Verification

Core and editor tests pass. The application-flow suite exercises real window
handlers, mode switching, empty-song editing, unsaved preview, 4:3 default after
resize, optional wide margins, original 16-bit prompt loading, Chance window,
Delete activation, pink-note input and restart reset. Layout images use GPU
readback before Present combined with native control printing; they are not
claimed to be desktop compositor screenshots. The legacy render_tests.exe
previously returned Access is denied; no security bypass was used.

Regression coverage explicitly starts from a 1024x768 client, enters and exits
Editor, compares both window/player rectangles, then enlarges the window and
checks that the toggle stays 640 pixels wide.
