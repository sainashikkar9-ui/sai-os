@echo off
setlocal

cd /d "%~dp0"

title Krishna OS Builder

echo ==========================================
echo       KRISHNA OS NATIVE TERMINAL
echo ==========================================
echo.

echo [1/4] Checking for G++ compiler...

where g++ >nul 2>&1

if errorlevel 1 (
    echo ERROR: G++ was not found in PATH.
    echo.
    echo Checking the expected compiler location...

    if exist "D:\mingw32\bin\g++.exe" (
        echo G++ exists at D:\mingw32\bin\g++.exe
        echo Using the full compiler path...
        set "CXX=D:\mingw32\bin\g++.exe"
    ) else (
        echo ERROR: G++ was not found at D:\mingw32\bin\g++.exe
        echo Verify your MinGW installation.
        pause
        exit /b 1
    )
) else (
    set "CXX=g++"
)

echo.
echo [2/4] Checking compiler version...
"%CXX%" --version

if errorlevel 1 (
    echo ERROR: Unable to execute the compiler.
    pause
    exit /b 1
)

echo.
echo [3/4] Building Krishna OS...

"%CXX%" -std=c++17 krishna_gui.cpp -o KrishnaOS.exe -mwindows -luser32 -lgdi32

if errorlevel 1 (
    echo.
    echo BUILD FAILED.
    echo Check the compiler errors displayed above.
    pause
    exit /b 1
)

echo.
echo [4/4] Build successful!
echo Launching Krishna OS...

start "Krishna OS" "%~dp0KrishnaOS.exe"

endlocal
exit /b 0