# Final HUD: full-canvas native draw

The release skin is assets/final_hud. It overrides Nexus and original aliases;
those layers only supply missing textures. All source DDS bytes are preserved.

Reference: supplied OldVTC SHA256
398b1e1c486b1ae673f49a39734deaa32bcf3e1840a5f4b0cc5dc4bd1976d981.
Reproduce with tools/inspect_native_hud_draw.py (pefile + capstone).

- 444C9C..444D08: normal resource draw reads descriptor +8 and +0xC into width
  and height, obtains texture through 45C4B0, calls 43F930.
- 43F9DA..43F9F9: quad dimensions are descriptor width/height multiplied by
  the rendering scale. This path does not inspect/crop alpha bounds.
- A0BEBA..A0BEC1 sets centered anchor mode for judgement/combo.
- A0C0BB..A0C14B uses x=529-16*digitCount and advances by 32 per digit;
  the original hit pulse applies to the full texture, not glyph fitting.
- 8EB879..8EB891 draws song_info_base through the same full-canvas helper.

Simulator corrections follow these observed draw semantics. In particular,
final song_info_base is a 1024x64 canvas with content only at x32..238; drawing
it in the old 256x64 rectangle shrank that content by four. It now uses its full
1024x64 size. Combo assets are 128x128 and retain that canvas, native anchors,
32px advance and existing hit scale. Judgements, Ready/Start, song-info layers
and Up likewise use source sizes where previous code assumed a fixed canvas.
No normal HUD texture is cropped to alpha bounds or stretched into a target box.
This trace covers the normal resource draw path, not every loading/sliced-font
variant or every native mode. Existing combo colour thresholds remain unchanged.

The user identified countdown art as coming from a different source. Those
three images alone have packaged alpha bounds and a uniform 64px glyph-height
placement at the existing countdown anchor; no other image uses that fitting.

The GDI-scaled DPI override remains removed. Monitor work-area sizing stays.
Validation includes the real product startup test, full-canvas resource size
checks and GPU captures of final HUD gameplay, Chance and 3/2/1.

The 100/400 BeatUp banners also use source canvas dimensions. The selected
Nexus fallback banners are 512x256; the previous fixed 512x128 draw halved
their height. Entry/blink/exit timing and anchors are retained. GPU milestone
captures exercise exactly 100 and 400 combo.
