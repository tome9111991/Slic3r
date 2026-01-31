@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo       Slic3r Build Script (VS 2022)
echo ==========================================

:: --- Configuration ---
set "VCPKG_URL=https://github.com/microsoft/vcpkg.git"
set "BASE_DIR=%~dp0"
set "VCPKG_DIR=%BASE_DIR%vcpkg"
set "BUILD_DIR=%BASE_DIR%build-msvc"
set "SOURCE_DIR=%BASE_DIR%src"

:: Dependencies list
set "DEPS=boost-system boost-thread boost-filesystem boost-nowide wxwidgets"

:: --- Checks ---
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found in PATH.
    pause
    exit /b 1
)

where git >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Git not found in PATH.
    pause
    exit /b 1
)

:: --- Vcpkg Setup ---
if not exist "%VCPKG_DIR%" (
    echo [INFO] Cloning vcpkg...
    git clone %VCPKG_URL% "%VCPKG_DIR%"
)

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo [INFO] Bootstrapping vcpkg...
    pushd "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat
    popd
)

:: --- Install Dependencies ---
echo [INFO] Installing dependencies via vcpkg (x64-windows)...
"%VCPKG_DIR%\vcpkg.exe" install --triplet x64-windows %DEPS%
if %errorlevel% neq 0 (
    echo [ERROR] Dependency installation failed.
    pause
    exit /b 1
)

:: --- CMake Configure ---
echo [INFO] Configuring CMake for Visual Studio 2022...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -S "%SOURCE_DIR%" -B "%BUILD_DIR%" ^
    -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DEnable_GUI=ON ^
    -DSLIC3R_STATIC=ON

if %errorlevel% neq 0 (
    echo [ERROR] CMake Configuration failed.
    pause
    exit /b 1
)

:: --- Build ---
echo [INFO] Building project (Release config)...
cmake --build "%BUILD_DIR%" --config Release --parallel

if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Build complete!
echo Executable: %BUILD_DIR%\Release\slic3r.exe
echo.
pause
