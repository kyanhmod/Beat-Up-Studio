Historical development notes; the current build uses assets/default.pack.

# Audition Beat-Up Simulator / Editor

Native C++ / Direct3D 11 / XAudio2 project intended to replace CdiuBeatUp's
player and editor on modern Windows machines. **This is still a simulator
prototype with a first native chart editor; the full Cdiu toolset and native
gameplay parity are not complete.**

## Reference

Use `(Nhi)Audition_OldVTC.exe` for the legacy HUD textures supplied here.
Cdiu source is an external reference for editing tools and file compatibility;
it is no longer stored in this project. Generated source inventories remain in docs. See [the traced addresses and replacement inventory](docs/OLD_AUDITION_REFERENCE.md).

## Working features

- Six directional lanes and Space, manual play and autoplay.
- Native SLK editor: tick grid, add/remove notes, range selection, copy/cut/paste,
  undo/redo, save edited copy and play it through the existing game engine.
  See [editor controls and limits](docs/EDITOR_FIRST_PASS.md).
- Horizontal Cdiu-style timeline with directional arrows, separate Space/Finish
  rows, overview navigation, zoom, toolbar, timing fields and transformation tools.
- Finish Move banner uses the supplied texture and Cdiu's entry/blink/exit timing.
- File menus and visible Open SLK/Choose music controls; independent music/effect
  volume sliders with saved levels. [Source feature inventory](docs/CDIU_FEATURE_INVENTORY.md)
  tracks the remaining Cdiu migration.
- Audition SLK import, including n/s/f combinations; separate TIME/LANE/TYPE
  demo charts use absolute milliseconds. Finish flags drive the Cdiu Finish Move
  banner; full native character choreography is not implemented.
- WAV (PCM / float) and OGG Vorbis playback. Duration comes from decoded audio.
- Per-chart UTF-8 song information, BPM and offset.
- OldVTC song_info layer order, coordinates and sprite dimensions reconstructed
  from static disassembly. Native phase progression and visual parity remain
  to be verified against the matching client.
- Unicode song labels, clipped to the HUD's label area.
- Restart resets the audio clock, and failed chart loads preserve the current chart.
- Space gauge fills B/E/A/T/U/P at Cdiu's combo thresholds, with orange/blue
  flames at 100/400, milestone banners and three combo-digit colors.
  See [combo display details](docs/SPACE_COMBO_REFERENCE.md).

The user-approved manual-input feel is the baseline for future Cdiu feature
migration; see [the preservation record](docs/INPUT_FEEL_BASELINE.md).

## Open a song

Run `dist/BeatUpStudio.exe` (one portable EXE). DDS and effect
audio are compressed inside the executable; no assets/SOUND folders or VC runtime
installation are needed. User songs/charts stay external and editable.

Run `bin/BeatUpStudio.exe` during development. It starts with no song selected. Use File → Choose
music, enter BPM/native offset in Default Setting, then Save. Import SLK through
File → Open SLK, or draw notes through Mode → Editor / Classic (F6). Music plays
with an empty chart too. Selection/seeking never auto-start playback. F9 pauses/
resumes; F10 toggles the embedded bar. Editor F5 previews unsaved notes.
See [native sequence and transport details](docs/NATIVE_SEQUENCE_AND_TRANSPORT.md).

Drag an SLK onto the window. Place same-stem `.ogg` or `.wav` beside the chart,
OR add `name.song.ini` beside `name.slk`:

```ini
[song]
title=V (conclusion)
artist=DJ TAKA
bpm=150
audio=music.ogg
offset_ms=0
```

The file is UTF-8; audio paths are relative to the chart. Positive offset delays
notes relative to music. Missing audio plays the chart with a metronome.
For an explicit native clock-phase conversion on Location charts, an optional
`audition_offset=0.7` is available; its units and index origin differ from
`offset_ms`. See [the native offset notes](docs/NATIVE_OFFSET_AND_HUD.md)
before converting a Cdiu list. Existing songs retain their timing by default.
`songs/v.song.ini` references the root OGG; `songs/v.wav` remains a local
test fixture and is excluded from the release. This metadata is specific to this simulator, not Audition.

## Controls

| Key | Action |
| --- | --- |
| Numpad 7/4/1 and 9/6/3 | Directional notes |
| Numpad 0 / Numpad 5 | Space note (keypad also works with NumLock off) |
| F1 or Tab | Switch key scheme |
| Q/A/Z and P/L/M (also I/K/comma) | Alternate directional keys |
| F2 | Autoplay |
| F3 | Key labels |
| F4 | VSync |
| F5 | Switch Black / bg.dds background (Black initially; also in Background menu) |
| F6 | Open the native chart editor |
| Ctrl+O | Open song files: OGG/WAV and/or SLK; multiple selection supported |
| Up/Down or +/- | Scroll speed |
| Left/Right | Seek backward/forward one beat |
| Space | Play/pause/resume in both player and editor |
| F9 | Pause/resume |
| F10 | Show the transport toolbar |
| R | Restart |
| F11 | Borderless fullscreen |
| Escape | Exit |

Motion and judgement now share the pixel model traced from Audition OldVTC.
Direction speed is 324 pixels per 8 sixteenth ticks; Space is 134 per 8 ticks.
Judgement thresholds and Space's asymmetric late boundary are documented in
[the gameplay trace](docs/GAMEPLAY_REFERENCE.md). Buffered key timestamps,
NumLock-off keypad mapping and a preallocated effect pool are supported.
The 4:3 playfield keeps its aspect ratio in widescreen windows/fullscreen.
Native chance/finish rules, effect details and end-to-end latency remain to be
validated. No 100% native fidelity is claimed.

## Build and check

Run `build.bat` using the configured MSVC installation. Output:
`bin/BeatUpSimulator.exe`. Keep its assets and songs folders alongside it.

Run `tests/run.bat` for real audio decoding, duration, restart clock, malformed
input, Unicode metadata and chart checks, plus hidden-window D3D rendering.
Tests use the supplied song files and the Windows audio/graphics devices.
Render captures are written under `tests/runtime`.

OGG decoding uses the complete bundled [stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c),
whose license is included in that source file.

Player and editor now share the main window. The transport is embedded at the
top right in a compact cluster. Play changes to Pause/Resume; F9 or Pause toggles playback. Seeking
retains combo and score; Restart clears them. Drag across the editor grid to
select ticks for Copy/Cut/Delete; a click still adds or removes a note.

The session now follows Cdiu: File → Choose music → enter BPM/offset → Save.
Music can play before any SLK exists (ALL COMBO = 0). Import a chart via File →
Open SLK, or use Mode → Editor / Classic (F6) to draw one. Mode → Game and
Autoplay switch views. Music Player → Toggle Menu On / Off (F10) controls the
embedded bar. Editor F5 previews unsaved notes without requiring a SLK file.
See docs/CDIU_SESSION_WORKFLOW.md for the source references and validation.

The workspace uses one compact top-right toggle, a player/editor area and
file/edit menus with a shared dark theme. Editor arrows use the game DDS art.
4:3 is the default even in fullscreen; View → Wide is optional. READY/START
voice phase timing is corrected. The native 16-bit Del Chance prompt now loads
and appears in its eligible pre-Finish window. See docs/UI_REFACTOR_AND_LAYOUT.md.

The toggle is now one compact cluster at the top right; file actions remain
in menus. Editor no longer enlarges the main window on entry. Mode changes keep
the window/player bounds stable; F10 still toggles the cluster.

Widescreen mode has been removed: gameplay always keeps 4:3, with black margins
on other window sizes. Key assist starts off; F3 toggles it on when needed.

The title bar shows only Beat-Up Studio. The compact icon transport contains
playback/seek/gains and time; modes stay in the Mode menu. Hover an icon for
its action and shortcut.

## Portable packaging (2026-10-16)

`build.bat` builds BeatUpStudio.exe with a static C++ runtime. Python + Pillow
prepare the icon and resource bundle; MSVC rc embeds them. The pack uses Windows
XPRESS Huffman, expands once in memory, and never extracts DDS/sounds to disk.
Original assets/textures DDS files take priority, including byte-identical
portable aliases; bg.dds supplements the missing background. All supplied DDS
textures are bundled; PNG preview duplicates and removed widescreen extras are
excluded. See build/asset_manifest.json for source hashes. The icon is a91.dds.
The transport shows current/total combo and measured loop FPS. Editor rows adapt
to window height; navigation uses the overview, wheel and keyboard, without the
native horizontal scrollbar. See docs/RELEASE_AND_CDIU_COMPARISON.md.

Sample OGG/WAV files were removed at the owner’s request. Load your own music.
Song integration tests require restoring the named external audio fixtures; the
application build requires no sample music or Cdiu checkout. Effect sources live
in assets/sound and are embedded during packaging.

Current skin is assets/final_hud, with Nexus/original fallback for missing
textures. F10 globally toggles transport regardless of focus;
F8 and its redundant volume menu were removed. Red Chance stays left; activation
adds the skin’s selected icon at the opposing right position. Unchanged editor
sync preserves session stats. Manual seek allows a full arrow approach before
automatic misses apply to newly encountered notes; hit windows are unchanged.

Startup automatically fits a 4:3 client inside the Windows monitor work area,
accounting for taskbar and display scaling. Smaller windows use a second editor
toolbar row. Resolution/work-area changes keep normal windows on-screen.
See docs/WINDOW_SIZING.md for behavior and verification limits.

Release skin: assets/final_hud, with Nexus/original fallback for missing files.
Normal HUD sprites retain their source canvas and native anchors/scales. Only
the three custom countdown images use glyph placement. See
docs/FINAL_HUD_NATIVE_DRAW.md for the verified OldVTC draw path.
