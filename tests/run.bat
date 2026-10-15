@echo off
setlocal
cd /d "%~dp0.."
call tools\msvc_env.bat
if errorlevel 1 exit /b 1
if not exist tests\runtime mkdir tests\runtime
cl /nologo /EHsc /std:c++17 /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /I src /Fo:tests\runtime\ /Fe:tests\runtime\core_tests.exe tests\core_tests.cpp src\SongInfo.cpp src\SlkParser.cpp src\AudioEngine.cpp src\VorbisDecoder.cpp /link xaudio2.lib ole32.lib
if errorlevel 1 exit /b 1
tests\runtime\core_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /I src /Fo:tests\runtime\ /Fe:tests\runtime\render_tests.exe tests\render_tests.cpp src\BeatUpGame.cpp src\PlaybackPanel.cpp src\Renderer.cpp src\DDSLoader.cpp src\SongInfo.cpp src\SlkParser.cpp src\AudioEngine.cpp src\VorbisDecoder.cpp /link d3d11.lib dxgi.lib d3dcompiler.lib gdi32.lib user32.lib xaudio2.lib ole32.lib comctl32.lib
if errorlevel 1 exit /b 1
tests\runtime\render_tests.exe
if errorlevel 1 exit /b 1
call tests\run_editor.bat
exit /b %errorlevel%
