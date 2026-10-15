@echo off
setlocal
cd /d "%~dp0.."
call tests\run_intro_presentation.bat
if errorlevel 1 exit /b 1
call tests\run_window_sizing.bat
if errorlevel 1 exit /b 1
call tests\run_startup.bat
if errorlevel 1 exit /b 1
call tests\run_transport_skin.bat
exit /b %errorlevel%
