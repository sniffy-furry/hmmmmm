@echo off
setlocal EnableDelayedExpansion

:: ============================================================
:: NFSMWToolkit - Windows build script
:: Requirements: CMake >= 3.25, Visual Studio (MSVC), vcpkg
:: Usage:
::   build.bat              -> Release build
::   build.bat Debug        -> Debug build
::   build.bat clean        -> Delete build\ directory
:: ============================================================

set CONFIG=%~1
if /i "%CONFIG%"=="clean" goto :clean
if "%CONFIG%"=="" set CONFIG=Release
if /i not "%CONFIG%"=="Debug" if /i not "%CONFIG%"=="Release" (
    echo [ERROR] Unknown config "%CONFIG%". Use Release or Debug.
    exit /b 1
)

:: --- Locate vcpkg -----------------------------------------------------------
set VCPKG_CMAKE=
if defined VCPKG_ROOT (
    set VCPKG_CMAKE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
) else (
    :: Common fallback locations
    for %%P in (
        "%USERPROFILE%\vcpkg"
        "%USERPROFILE%\source\vcpkg"
        "C:\vcpkg"
        "C:\src\vcpkg"
        "C:\tools\vcpkg"
    ) do (
        if exist "%%~P\scripts\buildsystems\vcpkg.cmake" (
            set VCPKG_CMAKE=%%~P\scripts\buildsystems\vcpkg.cmake
            set VCPKG_ROOT=%%~P
            goto :vcpkg_found
        )
    )
    echo [ERROR] vcpkg not found. Set VCPKG_ROOT or install vcpkg to a standard location.
    echo         e.g.  set VCPKG_ROOT=C:\vcpkg
    exit /b 1
)
:vcpkg_found
echo [INFO] Using vcpkg at %VCPKG_ROOT%

:: --- Space-free install root ------------------------------------------------
:: FFmpeg's configure/build cannot handle spaces in paths (LNK1117). This project
:: lives under "Need for Speed - Most Wanted\..." (spaces), so we redirect the
:: vcpkg install tree next to the (space-free) vcpkg root instead of the default
:: <project>\vcpkg_installed.
set "VCPKG_INST=%VCPKG_ROOT%\..\nfsmw_vcpkg_installed"
echo [INFO] vcpkg install root: %VCPKG_INST%

:: --- Release-only triplet ---------------------------------------------------
:: Custom triplet (triplets\x64-windows-rel.cmake) sets VCPKG_BUILD_TYPE=release
:: so heavy from-source ports (ffmpeg) don't also build an unused Debug variant.
set "VCPKG_TRIPLET=x64-windows-rel"
set "VCPKG_OVERLAY=%~dp0triplets"
echo [INFO] vcpkg triplet: %VCPKG_TRIPLET% (release only)

:: --- Install dependencies ---------------------------------------------------
echo [INFO] Installing vcpkg dependencies...
pushd "%~dp0"
"%VCPKG_ROOT%\vcpkg.exe" install --triplet %VCPKG_TRIPLET% --overlay-triplets "%VCPKG_OVERLAY%" --x-install-root="%VCPKG_INST%"
if errorlevel 1 (
    echo [ERROR] vcpkg install failed.
    popd
    exit /b 1
)

:: --- Fetch vendored single-file headers (if missing) ------------------------
echo [INFO] Checking vendored headers...

if not exist "vendor\miniaudio" mkdir "vendor\miniaudio"
if not exist "vendor\miniaudio\miniaudio.h" (
    echo [INFO] Downloading miniaudio.h ...
    curl -fsSL "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h" ^
        -o "vendor\miniaudio\miniaudio.h"
    if errorlevel 1 (
        echo [ERROR] Failed to download miniaudio.h. Check your internet connection.
        popd
        exit /b 1
    )
    echo [OK] miniaudio.h downloaded.
) else (
    echo [INFO] miniaudio.h already present, skipping.
)

if not exist "vendor\minimp3" mkdir "vendor\minimp3"
if not exist "vendor\minimp3\minimp3.h" (
    echo [INFO] Downloading minimp3.h ...
    curl -fsSL "https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h" ^
        -o "vendor\minimp3\minimp3.h"
    if errorlevel 1 (
        echo [ERROR] Failed to download minimp3.h. Check your internet connection.
        popd
        exit /b 1
    )
    echo [OK] minimp3.h downloaded.
) else (
    echo [INFO] minimp3.h already present, skipping.
)

:: --- Configure --------------------------------------------------------------
echo [INFO] Configuring (%CONFIG%)...
cmake -B build ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_CMAKE%" ^
    -DVCPKG_INSTALLED_DIR="%VCPKG_INST%" ^
    -DVCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET% ^
    -DVCPKG_OVERLAY_TRIPLETS="%VCPKG_OVERLAY%" ^
    -DCMAKE_BUILD_TYPE=%CONFIG% ^
    -DNFSMW_BUILD_GUI=ON
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    popd
    exit /b 1
)

:: --- Build ------------------------------------------------------------------
echo [INFO] Building (%CONFIG%)...
cmake --build build --config %CONFIG% --parallel
if errorlevel 1 (
    echo [ERROR] Build failed.
    popd
    exit /b 1
)

popd
echo.
echo [OK] Build complete. Binary: build\bin\%CONFIG%\MWTools.exe
exit /b 0

:: --- Clean ------------------------------------------------------------------
:clean
echo [INFO] Removing build\ directory...
if exist "%~dp0build" (
    rmdir /s /q "%~dp0build"
    echo [OK] Cleaned.
) else (
    echo [INFO] Nothing to clean.
)
exit /b 0
