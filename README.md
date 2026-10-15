# Beat-Up Studio

A native Windows Beat-Up player and SLK editor built with C++17, Direct3D 11
and XAudio2. The player and editor share one window and one audio clock.

## Build

Requirements: Windows 10/11 x64, Python 3, and Visual Studio C++ Build Tools
with an x64 compiler and Windows SDK. No Python packages are needed for a
normal build. The build detects Visual Studio through vswhere, or uses an
existing x64 Native Tools prompt.

```bat
build.bat
```

Run `dist/BeatUpStudio.exe`. DDS textures, six sound effects and the icon are
embedded in that one EXE. User music and SLK files remain external.
For an unregistered/custom compiler installation, set `BEATUP_VCVARS` to its
`vcvars64.bat`, then run the same build command.

## Use

- Ctrl+O: choose OGG/WAV and/or SLK; paired drag/drop also works.
- Enter BPM and native offset, then Save. Music can play before adding notes.
- Space: play/pause/resume in both modes. Numpad 0/5: Space notes.
- F6: player/editor. F5: background in player; preview in editor.
- F2: autoplay. F3: key labels (off initially). F9: pause/resume.
- F10: show/hide transport. R: restart. F11: fullscreen.
- Editor: click to draw, drag to select; Ctrl+wheel to zoom; Ctrl+Z/Y and
  Ctrl+C/X/V for undo/redo and selection editing. Ctrl+S saves an edited copy.

Gameplay keeps a 4:3 canvas. Startup fits the monitor work area; the compact
transport overlays the playfield. Music and effect volumes are independent.

## Repository layout

| Path | Contents |
| --- | --- |
| src/ | Complete application code, including stb_vorbis |
| assets/default.pack | Original default HUD + sound + icon, one versioned file |
| tools/ | Resource preparation, optional pack authoring, build/research utilities |
| tests/ | Tests and Windows test runners |
| docs/ | Format, implementation and native-reference notes |

The GitHub version uses the **original default textures**, not Nexus/final_hud.
The pack is read and verified by Python standard-library tooling during build,
then embedded by Windows rc. Runtime resource loading expands it in memory;
DDS/sound files are not extracted at runtime. See [asset packaging](docs/ASSETS.md).

## Verification

```bat
tests\run_smoke.bat
```

The smoke suite uses generated silent audio and does not need sample songs,
raw asset directories or a Cdiu checkout. It covers actual product startup,
window sizing, presentation, real-audio seek/resume and combo preservation.
Additional older song-specific suites require the external fixtures described
in [testing](docs/TESTING.md). No sample music or client executable is included.

The full Cdiu toolset and native parity are still in progress. BPM/speed events,
additional chart formats, library/recovery and some editing tools remain open.
Historical skin experiments in docs are reference notes, not the shipped skin.

## Notices

See [third-party notices](THIRD_PARTY_NOTICES.md). This snapshot does not choose
a new license for the project code or grant a blanket license to supplied game
assets. stb_vorbis retains the license text in its source.
