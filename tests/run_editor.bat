@echo off
setlocal
cd /d "%~dp0.."
call tools\msvc_env.bat
if errorlevel 1 exit /b 1
if not exist tests\runtime mkdir tests\runtime
cl /nologo /EHsc /std:c++17 /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /I src /Fo:tests\runtime\ /Fe:tests\runtime\editor_tests.exe tests\editor_tests.cpp src\ChartDocument.cpp src\ChartEditor.cpp src\SongInfo.cpp src\SlkParser.cpp src\AudioMixer.cpp src\AudioEngine.cpp src\VorbisDecoder.cpp /link gdi32.lib user32.lib comdlg32.lib comctl32.lib xaudio2.lib ole32.lib
if errorlevel 1 exit /b 1
tests\runtime\editor_tests.exe
exit /b %errorlevel%
