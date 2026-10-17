@echo off
cd /d "%~dp0"
title Krishna OS Builder
echo Building Krishna OS Native Terminal...
g++ -std=c++17 krishna_gui.cpp -o KrishnaOS.exe -mwindows -luser32 -lgdi32
if errorlevel 1 (
    echo.
    echo Build failed. Make sure MinGW-w64 g++ is installed and available in PATH.
    pause
    exit /b 1
)
start "Krishna OS" "%~dp0KrishnaOS.exe"
