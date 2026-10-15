# Tests

Run tests/run_smoke.bat for the fixture-free suite: presentation, window sizing,
actual product startup and generated-audio transport/editor integration.
Individual runners can also be called separately. All runners detect the MSVC
installation with tools/msvc_env.bat; output stays in ignored tests/runtime/.
Desktop compositor pixels and VM-specific GPU/DPI combinations are not fully
covered by synthetic bounds or GPU readback/control-composition images.

Legacy suites run_core, run_editor, run_session and run.bat use local music and
charts from development. They are retained as source but excluded from the
smoke command. They need the original Sai Gon Night (Gucci Remix) SLK/OGG and/or
songs/v.wav and v.slk as named in those tests. No such songs are distributed in
this source snapshot. Native reverse-engineering scripts likewise require an
external matching Audition client and optional pefile/capstone packages.
