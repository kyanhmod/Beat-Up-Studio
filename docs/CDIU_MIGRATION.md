# Cdiu feature migration

Product direction: replace the useful player/editor functionality with native
Windows code, retaining the manual-input feel accepted on 2026-10-15. Do not
port VB6/DirectX8 input timing into the new player. Source references describe
behavior and file formats; the modern engine remains the preview/play backend.

## Current delivered behavior

- Approved manual play, timing, note motion and hit feedback baseline.
- OGG/WAV, per-song information, chart BPM/offset, real music duration.
- Native-style song_info composition and aspect-preserving fullscreen.
- Space combo cells, orange/blue flames, milestone banners and digit colors
  ported from Cdiu's combo display.
- First native editor: SLK document, Korean headers, grid note editing,
  selected-tick copy/cut/paste, undo/redo, edited-copy save/reopen and full-song
  preview. Tested with Sai Gon Night at BPM 135 and native offset 0.7.
  See [controls and current limits](EDITOR_FIRST_PASS.md).
- Cdiu-style horizontal arrow timeline, overview/zoom, dedicated
  Space/Finish rows and finish authoring; Finish Move banner in player.
- Visible file/music controls and menus, Save as, editable BPM/native offset,
  separate persistent music/effect levels, basic validation, mirror, tick shift,
  direction randomization and selected-range Space generation.
- Empty startup and explicit transport: file/timing selection, sample-based seek,
  Pause/Resume/Restart, inline music/effect sliders, editor cursor preview/playhead.
- Audition Old combo-loss level state and READY/START/3-2-1 sequence, using
  original countdown DDS assets and SOUND OGG effects.

The authoritative migration checklist is generated from all 74 actual menu
entries and handlers in [CDIU_FEATURE_INVENTORY.md](CDIU_FEATURE_INVENTORY.md).
Track functionality there without asking the user to enumerate Cdiu features.

## Remaining work, in dependency order

1. **Extend chart document and safe save/reopen.** The basic native path is
   delivered. Extend compatibility beyond tested cell-based SLK files. Keep the original tick positions,
   n/s/f combinations and song metadata separate from live hit/miss flags.
   Validate import/export against Cdiu `cma3.bas` and bundled charts. Preserve
   unsupported fields before claiming a lossless round trip.
2. **Extend timeline editor.** The first grid/snap, add/delete, range selection, copy/cut/paste,
   undo/redo and saved/dirty state are delivered. Add lane-specific selection and
   richer tools. Reproduce the editing behavior in `cma5.bas`
   using the document model, not mutations of the live playback state.
3. **Extend preview transport.** Pause/resume, seek and cursor preview are delivered.
   Add Space-only navigation and section looping. Preview must continue using the accepted player engine.
4. **Cdiu timing and transformation tools.** BPM segments, space generation,
   shift/mirror/randomize, selection transformations and chart validation.
5. **Library and recovery.** Song lists, title/artist/author/level, configurable
   folders, settings and autosave/recovery.
6. **Additional formats.** Evaluate KBE/DDR/CBE/CBG import/export after the
   SLK/document round trip is reliable. Do not silently discard unavailable
   embedded audio or unsupported attributes.
7. **Remaining gameplay presentation.** Chance toggle, finish sequences,
   full gauge/score transitions, native audio effects and clean backgrounds.

For each increment, run the existing input/clock regression suite as well as
checks specific to that feature. A working combo gauge is not a completed editor
or a completed Cdiu replacement. Legacy online endpoints are outside the local
player/editor migration until explicitly scoped.
