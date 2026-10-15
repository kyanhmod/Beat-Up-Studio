# Nexus HUD and transport corrections

The embedded pack prioritizes assets/hud_nexus, matching original filenames,
case-insensitive Latin names, Korean/mojibake names and portable aliases using
the original-source hash identity. Notes/song-info/background absent from this
partial skin retain their existing assets. Build/asset_manifest.json records
all selected source files and hashes. No texture is recolored.

Chance uses 찬스버튼_3 (red C) on the left at (287,653). Activation adds
찬스버튼_2 at the mirrored right position (737,653), both native 64x64.
The grey disabled texture is no longer assigned to the ordinary red state.

F10 is handled before native menu dispatch, including WM_SYSKEYDOWN/UP, and is
independent of focus on the editor, seek or volume controls. Repeats/key release
cannot double toggle. F8 and the duplicate volume menu are removed; the toggle
still contains independent music/effect controls.

Seek retains scored statistics. Synchronizing an unchanged editor chart no
longer invokes SetBpm/Restart. In manual play, notes already in flight at the
seek destination remain hittable, but their expiration cannot immediately
break the old combo. The grace duration is one native direction approach;
normal expiry resumes after that boundary. Restart/new chart clears this grace.
Judgement thresholds and timestamped input are unchanged.

Validation: tests/run_transport_skin.bat generates its own temporary silent WAV,
seeds a four-digit combo, repeatedly seeks forward/backward through the actual
trackbar messages, resumes and advances real audio/game ticks. It also checks
unchanged-chart sync, manual landing grace, global F10 system-key semantics,
Nexus source bytes, icon states, and produces player/Chance render captures.
It requires no removed sample songs or Cdiu checkout.

Countdown presentation retains source aspect at 64px height and the existing
countdown anchor. Nexus countdown textures are 324x64 with transparent side
padding; the full canvas must not be squeezed into the old 32/64px width.
ChancePromptSlide preserves its current X when availability ends, then slides
left until the 256px prompt is fully offscreen. Activation and timeout share
this exit path; pause freezes animation with song time. Restart/seek clears it.
The existing 500ms entrance remains; a full exit also takes 500ms.
Validated by tests/run_intro_presentation.bat and GPU countdown/exit captures.
