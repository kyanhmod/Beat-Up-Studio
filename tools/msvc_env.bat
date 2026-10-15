@echo off
rem Keep environment changes in the calling build/test script.
if defined BEATUP_VCVARS (
    if exist "%BEATUP_VCVARS%" (
        call "%BEATUP_VCVARS%" > nul
        if errorlevel 1 exit /b 1
        exit /b 0
    )
)
where cl.exe >nul 2>nul
if not errorlevel 1 (
    where rc.exe >nul 2>nul
    if not errorlevel 1 exit /b 0
)
set "BEATUP_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%BEATUP_VSWHERE%" (
    echo [ERROR] Open an x64 Native Tools prompt, or set BEATUP_VCVARS to vcvars64.bat.
    exit /b 1
)
set "BEATUP_VS_ROOT="
for /f "usebackq delims=" %%P in (`"%BEATUP_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "BEATUP_VS_ROOT=%%P"
if not exist "%BEATUP_VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat" (
    echo [ERROR] Install Visual Studio C++ Build Tools and a Windows SDK.
    exit /b 1
)
call "%BEATUP_VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
exit /b %errorlevel%
