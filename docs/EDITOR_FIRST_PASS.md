# First native editor increment

The current executable is `bin/BeatUpStudio.exe`. It starts without a selected
song or playback; choose OGG/WAV from File, enter timing, then Save.
Mode → Editor / Classic (F6) works before importing or saving any SLK.
A chart path can also be supplied on the command line or dropped onto the player.
The supplied `.song.ini` sets BPM 135, native `audition_offset=0.7`, and the
original OGG. The original OGG and SLK are not modified.

## Using it

- F6 in the player pauses playback and opens the editor.
- The player has File / Open song files (Ctrl+O), Import SLK, Choose music, Music Player, Mode, Setting and Volume
  menus. The editor exposes Open SLK, Choose music, Save copy, Save as, Play,
  Volume and editing buttons directly on its toolbar.
- Click a grid cell to add/remove a note. Space occupies its own row.
  Time runs left to right, following Cdiu's editor layout, with arrow glyphs
  for the six direction rows and separate Space and Finish rows. The grid
  starts just before the first event. Bar/beat/subdivision labels use
  the chart's zero-based Location convention; one tick is a sixteenth note.
- Left/Right or Up/Down move the tick cursor; Home/End jump to the start/last
  event. Wheel, overview and Page Up/Down navigate the chart.
  Ctrl+wheel zooms. Click the song overview to jump. F7 cycles snap 1/16, 1/8
  and 1/4. Ctrl+A selects the whole chart.
- Number keys or Numpad 7/4/1/9/6/3 add/remove directions at the cursor.
  Numpad 0/5 add/remove Space, including NumLock-off keypad aliases.
  Space controls play/pause/resume and never inserts a note.
- Click the Finish row, use the Finish button or press F to add/remove the
  finish flag at the cursor; existing directions/Space are retained. The player
  displays the supplied Finish Move sprite with Cdiu's entry/blink/exit schedule.
- Shift+click or Shift+navigation selects an inclusive tick range across all
  lanes.
  Delete removes events in the selected range, including finish markers.
- Ctrl+C/X/V copy/cut/paste the selected range relative to the destination
  cursor. The clipboard is internal to this editor session. Identical row events
  are not duplicated on repeated paste. Ctrl+Z/Y undo/redo up to 100 changes.
- Ctrl+S saves `<name>.edited.slk` and matching song information beside the
  source. An existing edited copy prompts before its first overwrite; an opened
  `.edited.slk` saves directly to itself. A star marks unsaved document changes.
- F5 previews the current in-memory chart from the cursor with one beat of lead-in,
  keeping the editor open and following a moving playhead. Preview uses autoplay
  with Pause/Resume/Restart and a seekable transport. F6/Escape closes the editor,
  pauses preview and restores the previous manual/autoplay setting.
  Closing with changes offers Save/Discard/Cancel. Ctrl+O opens the shared song-file picker.
- BPM and native offset have editable fields and an Apply timing button.
  Saving/playing also validates and applies those fields. Audio chosen in the
  player is inherited when opening its editor; Choose music in the editor
  persists the selected OGG/WAV reference with the edited chart.
- Tools provides selected-range mirror, earlier/later tick shift, direction
  randomization preserving chord count, Space generation every 16 ticks and
  basic event/duplicate/finish validation. Changes participate in undo/redo.
- Volume (F8 in the player) opens separate music/effect sliders. Levels are
  applied live and stored in `bin/BeatUp.settings.ini`, surviving song changes,
  restart and app relaunch.
- The transport also exposes both volume sliders inline. BPM/offset/music
  entered in the player are inherited when opening the editor, even before
  pressing Play; choosing music does not erase pending timing fields.

## Compatibility and scope

The format reference is Cdiu `cma3.bas:SaveSlkSub`, which writes six columns
and `n/s/f` flags. Editing operations follow the selected-tick behavior in
`cma5.bas:GoCopy`, paste and undo routines, using a separate native document.
This is a behavior port; Cdiu's VB6 input clock is not used.

The player/editor now recognizes the Korean native header through the typed
second row (`int`, `enum(n,s,f)`, `string`), alongside English header charts.
Unchanged saves reproduce original bytes, including Korean header encoding.
Edited saves retain raw header/unknown cell bytes, update the six known fields,
keep combined Space/finish flags when one direction is removed, and write
events in tick order. Files are written to a temporary file and replaced after
the write succeeds. Unsupported columns remain in the table when their event
is removed. This is not a general spreadsheet/formula editor; arbitrary SYLK
formatting and row-referencing formulas are not covered by compatibility tests.

Remaining: lane-specific rectangle selection, a system clipboard/Cdiu clipboard
bridge, segment looping, waveform, BPM segments, chance and advanced
speed authoring, autosave/recovery, remaining transformations and other formats.
TIME/LANE demo charts remain playable but are not supported for editing here.
The full source-derived checklist is in CDIU_FEATURE_INVENTORY.md; this release
does not claim complete Cdiu parity.

## Validation

`tests/run.bat` includes document and Win32 editor integration tests. Actual
Sai Gon Night checks cover native headers, 1620 events, byte-exact unchanged
save, edit/reopen, ordering, copy/cut/paste, undo/redo, flags and unknown columns.
The editor window test sends editing commands, renders its client area, saves
through F5 and validates the resulting chart, metadata and OGG reference.
The existing game tests also judge the actual song's first Space at the converted
135 BPM / 0.7 offset timestamp. Audio tests decode the supplied OGG.

Input rules remain unchanged from the accepted baseline. AudioEngine adds gain
controls around voice output; sample-clock interpolation and event timing are
unchanged. Tests cover live gain changes without clock reset, level persistence
through restart and the actual mixer sliders/INI reload.
Main input changes only add editor routing; ordinary manual play still uses the
existing queued-event timestamps, repeat gate and sample-interpolated clock.

Current session behavior supersedes the original first-pass saved-file preview:
F5 previews unsaved notes in memory. Mode changes keep the current unsaved chart
without a Save prompt. Only replacing a session/file or closing the app invokes
Save/Discard/Cancel. See [the source-driven workflow](CDIU_SESSION_WORKFLOW.md).
