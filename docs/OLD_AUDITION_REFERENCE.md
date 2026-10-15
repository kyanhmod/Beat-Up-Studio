# Audition Old reference and Cdiu replacement

## Reference policy

Use `(Nhi)Audition_OldVTC.exe` from `05_Reverse_Engineering/Game_Binaries`
for the supplied legacy textures. Use Cdiu source to inventory editing features
and file compatibility. Do not describe guessed values as native behavior.
`tools/inspect_old_song_info.py` reproduces the static HUD trace without running
or modifying the client. Other Old PH/CN builds have different addresses.

## Song information: observed in OldVTC

At 0x008EB820..0x008EB93C, the default layout calls:

| Element | Position / behavior | Evidence |
| --- | --- | --- |
| blank | (-16, 700), native 256x64 | 0x008EB82C..0x008EB84B |
| full | same origin; horizontal extent 105 * (phase / 105 + 0.6) | 0x008EB820..0x008EB874; helper 0x004405B3..0x00440649 |
| base | same origin, after full | 0x008EB879..0x008EB891 |
| bar | top-left (31 + integer phase, 731), native 32x32 | 0x008EB896..0x008EB8C0 |
| text | (33, 711), width parameter 160 | 0x008EB910..0x008EB91F |
| feather | same origin, after text | 0x008EB924..0x008EB93C |

The alternate branch at 0x008EB4A5 formats two strings and BPM as
`%s %s (%1.f bpm)`, with Gulim size 13. The countdown format at 0x00E7DE8C
is `%02d:%02d`; the alternate branch positions it at (160 + mode offset, 739).
The positioning helper uses virtual reference dimensions 1023 and 767.
Mode 0xE6 applies another horizontal offset; the simulator currently implements
the default layout only. This does not establish which mode identifier is Beat-Up.

**Still inferred:** mapping elapsed audio / duration onto phase 0..105. The native
phase's source and update behavior require a separate trace. The simulator uses
elapsed audio fraction. Font rasterization and time rounding are not proven exact.
The supplied textures may come from a different revision even though names match.
Visual comparison against the matching client remains necessary.

## Chart flags and asset caveats

`CdiuBeatUpEditor-master/cma3.bas`, `LoadToGData` (around lines 190..234),
tests n, s, f independently. n writes a directional lane, s writes Space,
and f writes a separate finish flag. The bundled V chart contains `s,f` rows.
The parser now preserves combinations instead of treating all flags as one type.
Finish-only rows are timeline events, excluded from playable-key judgement;
native finish choreography remains pending.

The hidden-window render test confirms that the current `bg_stage.dds` has
MISS, GET READY, characters and a Bandicam watermark baked into the picture.
Those are not all simulator draw calls. A clean stage resource is required for
faithful presentation; this pass does not alter the supplied background.

## Song metadata supported by the simulator

Optional UTF-8 file next to a chart: `name.song.ini` for `name.slk`.

```ini
[song]
title=V (conclusion)
artist=DJ TAKA
bpm=150
audio=V.ogg
offset_ms=0
```

Audio paths resolve relative to the chart. Without an explicit audio path, try
same-stem OGG then WAV. Duration comes from decoded audio; silent charts use the
last note plus its miss window. Positive offset_ms delays notes relative to audio.
Metadata is an emulator feature, not an assertion about Audition's metadata format.

## Replacement scope from Cdiu source

The menu strings in `cma5.bas` (roughly lines 550..650), together with `cma1.bas`
and `cma2.bas`, show functionality that must be considered for replacement:

| Area | Required work | Current state |
| --- | --- | --- |
| Playback | music, offset/BPM changes, play/stop, play from selected space | basic playback; no full transport yet |
| Gameplay | classic/preview/game modes, authentic notes, chance/space, scoring | approved manual-input baseline; Space combo visuals added; chance/full scoring pending |
| Editor | grid, place/delete notes, select/cut/copy/paste, undo, transformations | not implemented |
| Timing tools | BPM segments, automatic spaces, shift spaces, validation | not implemented |
| Files | SLK import/export; evaluate KBE/DDR/CBE/CBG compatibility | SLK import only; parser still needs full validation |
| Song library | artist/title/author/level/music code, audio/script folders, lists | per-chart metadata only |
| Recovery | temporary save/load, settings | not implemented |
| Appearance | backgrounds, UI scaling, language | 4:3 scaling, Unicode song labels, combo digit colors; screenshot stage optional |

Do not claim Cdiu replacement complete until editing, save/reopen without data
loss, playback transport, and gameplay comparisons are verified. Legacy online
rooms and upload endpoints need a separate product decision before reproducing.
