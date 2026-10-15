@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"
echo ======================================================================
echo           AUDITION BEAT-UP STANDALONE SIMULATOR BUILDER (x64)
echo ======================================================================

call tools\msvc_env.bat
if errorlevel 1 exit /b 1

echo [*] Compiler initialized.
if not exist "bin" mkdir "bin"
set "BUILD_EXE=BeatUpStudio.exe"
if not "%~1"=="" set "BUILD_EXE=%~nx1"

if not exist "build" mkdir "build"
python tools\package_assets.py
if errorlevel 1 exit /b 1
pushd build
rc /nologo /fo studio.res studio.rc
if errorlevel 1 (
    popd
    exit /b 1
)
popd

set CFLAGS=/MT /Fo:build\ /nologo /O2 /Oi /Oy /GL /Gy /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNDEBUG /I"src"
set LDFLAGS=/link /NOLOGO /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS d3d11.lib dxgi.lib d3dcompiler.lib user32.lib gdi32.lib ole32.lib xaudio2.lib shell32.lib comdlg32.lib comctl32.lib

echo [*] Compiling %BUILD_EXE% ...
cl %CFLAGS% /Fe:"bin\%BUILD_EXE%" src\*.cpp build\studio.res %LDFLAGS%

if errorlevel 1 (
    echo [ERROR] Build failed!
    exit /b 1
)

if not exist "dist" mkdir "dist"
copy /y "bin\%BUILD_EXE%" "dist\%BUILD_EXE%" > nul
if errorlevel 1 exit /b 1
echo [SUCCESS] Built self-contained dist\%BUILD_EXE%
exit /b 0
