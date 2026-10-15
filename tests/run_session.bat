@echo off
setlocal
cd /d "%~dp0.."
call tools\msvc_env.bat
if errorlevel 1 exit /b 1
if not exist tests\runtime mkdir tests\runtime
python tools\package_assets.py
if errorlevel 1 exit /b 1
pushd build
rc /nologo /fo studio.res studio.rc
if errorlevel 1 (
    popd
    exit /b 1
)
popd
cl /nologo /EHsc /std:c++17 /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /I src /Fo:tests\runtime\ /Fe:tests\runtime\session_flow_tests.exe tests\session_flow_tests.cpp build\studio.res src\AudioEngine.cpp src\AudioMixer.cpp src\BeatUpGame.cpp src\ChartDocument.cpp src\ChartEditor.cpp src\DDSLoader.cpp src\PlaybackPanel.cpp src\Renderer.cpp src\SlkParser.cpp src\SongInfo.cpp src\VorbisDecoder.cpp src\StudioShell.cpp /link /SUBSYSTEM:CONSOLE d3d11.lib dxgi.lib d3dcompiler.lib gdi32.lib user32.lib comdlg32.lib comctl32.lib xaudio2.lib ole32.lib shell32.lib
if errorlevel 1 exit /b 1
tests\runtime\session_flow_tests.exe
exit /b %errorlevel%
