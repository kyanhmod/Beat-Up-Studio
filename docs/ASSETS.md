# Single resource bundle

The repository tracks only assets/default.pack under assets/. It contains the
original default DDS skin, bg.dds, BEAT/SPACE/GREAT/MISS/READY/START OGG effects,
app.ico and asset_manifest.json. Nexus and final_hud are not part of this pack.

Normal build needs only Python standard library. tools/package_assets.py checks
resource hashes, copies the bundle to build/assets.pack and materializes the
icon/RC input in build/. The EXE embeds the entire pack as RCDATA resource 201
and the icon as resource 101. AssetBundle.h expands the pack once in memory.

Format: little-endian uint64 uncompressed payload length, followed by Windows
XPRESS-Huffman data (Compress API algorithm 4). The decompressed payload is a
sequence of uint32 UTF-8 name length, uint32 data length, name bytes and data
bytes. Filenames are basenames; paths and duplicate names are rejected by the
Python reader. The C++ reader and Python tool both cap expansion at 64 MiB.

Optional authoring, with Pillow installed from requirements-assets.txt:

```bat
python tools\create_asset_pack.py --textures path-to-normalized-dds --sounds path-to-effects --chance-active path-to-blue-chance.dds --output assets\default.pack
```

The textures directory uses the portable names referenced by BeatUpGame.cpp,
such as a91.dds, crazy.dds, track_l1.dds, combo_0.dds and countdown_1.dds.
Sound filenames are BEAT.ogg, SPACE.ogg, GREAT.ogg, MISS.ogg, READY.ogg, START.ogg.
The pack generator preserves DDS/OGG bytes and derives the icon from a91.dds.
No game/client executable, third-party checkout or raw asset tree is required
to clone and build the checked-in source snapshot.
