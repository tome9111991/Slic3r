@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set "VCPKG_URL=https://github.com/microsoft/vcpkg.git"
set "BASE_DIR=%~dp0"
if "%BASE_DIR:~-1%"=="\" set "BASE_DIR=%BASE_DIR:~0,-1%"

set "GLOBAL_BUILD_DIR=%BASE_DIR%\build"
set "DEPS_DIR=%GLOBAL_BUILD_DIR%\deps"
set "VCPKG_DIR=%DEPS_DIR%\vcpkg"
set "CMAKE_BUILD_DIR=%GLOBAL_BUILD_DIR%\target"
set "SOURCE_DIR=%BASE_DIR%\src"
set "LOG_FILE=%BASE_DIR%\build_log.txt"

:: Create directories
if not exist "%GLOBAL_BUILD_DIR%" mkdir "%GLOBAL_BUILD_DIR%"
if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

:: --- Checks ---
where cmake >nul 2>nul || exit /b 1
where ninja >nul 2>nul || exit /b 1

:: --- Find Visual Studio ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)
if not exist "%VS_PATH%" exit /b 1

:: --- Setup Environment ---
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1

:: --- CMake Configure & Build ---
echo [INFO] Configuring...
powershell -Command "& { cmake -S '%SOURCE_DIR%' -B '%CMAKE_BUILD_DIR%' -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE='%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake' -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_MANIFEST_DIR='%BASE_DIR%' -DEnable_GUI=ON -DSLIC3R_STATIC=OFF 2>&1 | Tee-Object -FilePath '%LOG_FILE%'; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }"

if %errorlevel% neq 0 (
    echo [ERROR] Configure failed.
    exit /b 1
)

echo [INFO] Building...
powershell -Command "& { cmake --build '%CMAKE_BUILD_DIR%' --parallel 2>&1 | Tee-Object -FilePath '%LOG_FILE%' -Append; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }"

if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [INFO] Copying resources...
if not exist "%GLOBAL_BUILD_DIR%\var" mkdir "%GLOBAL_BUILD_DIR%\var"
xcopy /E /I /Y "%BASE_DIR%\var" "%GLOBAL_BUILD_DIR%\var" >nul

echo [SUCCESS] Build complete.
exit /b 0
