@echo off
setlocal
cd /d "%~dp0.."
call tools\msvc_env.bat
if errorlevel 1 exit /b 1
if not exist tests\runtime mkdir tests\runtime
cl /nologo /EHsc /std:c++17 /I src /Fo:tests\runtime\ /Fe:tests\runtime\window_sizing_tests.exe tests\window_sizing_tests.cpp /link user32.lib
if errorlevel 1 exit /b 1
tests\runtime\window_sizing_tests.exe
exit /b %errorlevel%
