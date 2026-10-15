# Cdiu session workflow port

Source behavior read directly:

- `test.frm:openmusic_Click` → `cma1.bas:OpenSound` (358): decode music,
  show Frame1 settings, allocate an empty chart, select see/autoplay mode.
- `test.frm:Label_save_Click` (2267) → `cma2.bas:SaveData` (64): commit BPM
  and offset, hide settings; `CheckDFrame2` (54) toggles the embedded Frame2.
- `test.frm:SetNormalMode_Click` (2738), SetSeeMode and SetGameMode →
  `cma2.bas:UseModeChange` (292): explicit Editor/Autoplay/Game modes.
- `cma2.bas:CheckCombo` (275): total playable notes across seven key rows;
  Finish markers are not combo. `cma4.bas:Render` uses the same chart in all modes.

Native workflow:

1. File → Open song files (Ctrl+O) accepts OGG/WAV and SLK together in either
   selection order; drag/drop accepts the same pair. Choose music retains an
   existing chart. New song explicitly starts an empty session.
2. An embedded settings panel shows Main BPM and native tick offset. Save
   commits these values in the session and returns to the compact toggle bar.
3. Music can play with zero notes, before any SLK exists. ALL COMBO is zero.
4. File → Open SLK imports notes into the selected song, retaining its timing.
5. Mode → Editor / Classic (F6) works without a file. Drawing updates ALL COMBO.
   F5 previews the current in-memory notes without forcing a save dialog.
6. Mode → Game or Autoplay returns to gameplay with the current in-memory chart.
   Switching modes retains unsaved notes without prompting to save. Replacing a
   song/chart or closing the app uses Save/Discard/Cancel; SLK files are written
   only on an explicit save. A new chart uses Save as, never a hidden temp SLK.
7. Music Player → Toggle Menu On / Off (F10) toggles the embedded bar. Its
   controls are Play/Pause/Resume, Restart, settings, previous/next Space,
   previous/next beat, seek and independent music/effect levels.

The render surface and native panels are sibling child windows in one main
window; gameplay presentation does not draw over the native controls. The
accepted judgement rules and timestamped manual input path are unchanged.

Main BPM/offset are supported. Additional BPM slots and per-tick BPM/speed
rows in Cdiu remain separate migration work; they are not claimed complete.

Verification: `tests/run_session.bat` exercises actual main-window menu/key
handlers for music → settings → Save → zero-note play/pause → toggle → empty
editor → drawing → unsaved preview → Game → SLK import → editor → new music.
Its layout images combine actual GPU readback with native control printing;
desktop screen capture returns black in this environment, so these are not
claimed to be compositor screenshots.

Latest validation: application session flow and editor suites pass; core suite
passes. The separate legacy render_tests.exe linked but execution returned
Access is denied; it was not bypassed or reported as passed. Product build passes.

The toggle is one compact fixed-width top-right cluster in the shared dark
Studio shell. File operations stay in menus, and mode buttons are inside the
cluster. The editor adapts to the same window without automatic enlargement. See UI_REFACTOR_AND_LAYOUT.md for current 4:3/default behavior.

Current transport fixes: Space controls play/pause/resume in both modes;
Numpad 0/5 retain note entry. The toggle overlays the full canvas, with no
reserved strip. Seeking preserves score, combo, counts and visible judgement.
Timing fields only apply after an edit, avoiding a restart from rounded values.
Changing music in Editor restores Editor after timing setup is saved.
