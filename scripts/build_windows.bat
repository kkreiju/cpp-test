@echo off
REM ──────────────────────────────────────────────
REM build_windows.bat - Compile NCTV Player on Windows
REM ──────────────────────────────────────────────
REM Prerequisites:
REM   - CMake 3.20+
REM   - Qt6 installed (set Qt6_DIR or CMAKE_PREFIX_PATH)
REM   - VLC SDK extracted into external/vlc-sdk-windows/
REM   - Visual Studio 2022 Build Tools (MSVC)
REM ──────────────────────────────────────────────

setlocal enableextensions

set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build-windows

echo ═══════════════════════════════════════════
echo   NCTV Player - Windows Build
echo ═══════════════════════════════════════════

REM Clean previous build (optional)
if "%1"=="clean" (
    echo Cleaning previous build...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo.
echo [1/3] Configuring CMake...
cmake "%PROJECT_DIR%" ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)

echo.
echo [2/3] Building...
cmake --build . --config Release --parallel

if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo [3/3] Build successful!
echo Executable: %BUILD_DIR%\Release\nctv-player.exe
echo.
echo To run: %BUILD_DIR%\Release\nctv-player.exe
echo ═══════════════════════════════════════════

endlocal
