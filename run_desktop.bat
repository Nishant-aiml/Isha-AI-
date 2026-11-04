@echo off
title ISHA AI - Desktop Studio Edition v2.0
cd /d "%~dp0"
echo ========================================================
echo   ISHA AI - Desktop Studio Edition v2.0
echo   All-in-One AI Fiesta Architecture
echo   Launching Backend and Standalone Desktop Window...
echo ========================================================
python run_desktop.py
if errorlevel 1 (
    echo.
    echo Backend encountered an issue. Launching directly in browser...
    start http://127.0.0.1:8000
)
pause
