@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo    Slic3r VS Solution Generator
echo ==========================================

:: --- Configuration ---
set "VCPKG_URL=https://github.com/microsoft/vcpkg.git"
set "BASE_DIR=%~dp0"
if "%BASE_DIR:~-1%"=="\" set "BASE_DIR=%BASE_DIR:~0,-1%"

:: Using a separate directory for the solution to avoid conflicts with Ninja build
set "GLOBAL_BUILD_DIR=%BASE_DIR%\build"
set "DEPS_DIR=%GLOBAL_BUILD_DIR%\deps"
set "VCPKG_DIR=%DEPS_DIR%\vcpkg"
set "SLN_BUILD_DIR=%GLOBAL_BUILD_DIR%\sln"
set "SOURCE_DIR=%BASE_DIR%"

:: --- Colors ---
for /F "delims=" %%A in ('"prompt $E$S & echo on & for %%B in (1) do rem"') do set "ESC=%%A"
set "ESC=%ESC:~0,1%"
set "RED=%ESC%[91m"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "CYAN=%ESC%[96m"
set "RESET=%ESC%[0m"


:: Create directories
if not exist "%GLOBAL_BUILD_DIR%" mkdir "%GLOBAL_BUILD_DIR%"
if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

:: --- Find Visual Studio ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)
if not exist "%VS_PATH%" (echo %RED%[ERROR]%RESET% Visual Studio 2022 not found. & exit /b 1)

:: --- Setup Environment ---
echo %YELLOW%[INFO]%RESET% Setting up VC++ Environment...
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" || (echo %RED%[ERROR]%RESET% vcvars64.bat failed. & exit /b 1)

:: --- Vcpkg Setup ---
if not exist "%VCPKG_DIR%" (
    echo %YELLOW%[INFO]%RESET% Cloning vcpkg...
    git clone %VCPKG_URL% "%VCPKG_DIR%"
)
if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo %YELLOW%[INFO]%RESET% Bootstrapping vcpkg...
    pushd "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat
    popd
)

:: --- CMake Configure (VS 2022 Generator) ---
echo %YELLOW%[INFO]%RESET% Configuring CMake for Visual Studio 2022...
if not exist "%SLN_BUILD_DIR%" mkdir "%SLN_BUILD_DIR%"

:: Note: Multi-config generator (VS) ignores CMAKE_BUILD_TYPE at configure time.
:: You select Debug/Release inside Visual Studio.
cmake -S "%SOURCE_DIR%" -B "%SLN_BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_MANIFEST_DIR="%BASE_DIR%" -DEnable_GUI=ON -DSLIC3R_STATIC=OFF

if %errorlevel% neq 0 (
    echo %RED%[ERROR]%RESET% CMake Configuration failed.
    exit /b 1
)

echo.
echo %GREEN%[SUCCESS]%RESET% Solution generated!
echo Location: %SLN_BUILD_DIR%\Slic3r.sln
echo.
echo You can now open this file in Visual Studio 2022.
echo.
pause
