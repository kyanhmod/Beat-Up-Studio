# Space combo display

Ported from Cdiu `cma4.bas`:

- `CheckComboAdd` (around 1852): six orange cells at 10/20/40/60/80/100,
  six blue replacements at 150/200/250/300/350/400;
- `ShowCB` (around 1932): 48-pixel cell advance, 64x32 glow and 16x16 letter;
- `ShowY` / `ShowB` (around 2037): orange flame from 100, blue flame from 400;
- `CheckMoreCombo` / `ShowBS`: entering/flashing/exiting Beat Up banner over
  eight sixteenth ticks when crossing 100 or 400;
- `RenderBig` / `RenderSmall`: white/gray combo digits below 100, yellow from
  100, purple from 200.

All colors use supplied original DDS textures. `tools/prepare_combo_assets.py`
copies bytes to portable aliases; `assets/hud_clean/combo_assets.json` records
their original filenames and SHA-256 hashes. No textures are recolored.
`build.bat` packages all HUD DDS files into `bin/assets/hud_clean` so new textures
are present in the actual executable's preferred resource directory.

The display is derived from the current combo. A miss uses the existing combo
reset and therefore clears cells/flames; restart clears pending banners. A new
chain can trigger 100/400 again. A still-running banner does not restart on each
additional hit. A jump over a threshold also triggers it.

This ports Cdiu's combo-driven visual behavior. Audition Old's separate gauge
level-loss logic, chance flags and full score multipliers have not been added
in this change. The accepted input feel is preserved; see INPUT_FEEL_BASELINE.md.

Validation: pure boundary checks immediately below/at every threshold, a
400-note autoplay fixture, milestone expiration/retrigger, miss and restart,
and render captures for all threshold stages under `tests/runtime`.
