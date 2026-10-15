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

exit /b %errorlevel%
