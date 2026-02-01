@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo       Slic3r Build Script (Live-Log)
echo ==========================================

:: --- Configuration ---
set "VCPKG_URL=https://github.com/microsoft/vcpkg.git"
set "BASE_DIR=%~dp0"
if "%BASE_DIR:~-1%"=="\" set "BASE_DIR=%BASE_DIR:~0,-1%"

set "GLOBAL_BUILD_DIR=%BASE_DIR%\build"
set "DEPS_DIR=%GLOBAL_BUILD_DIR%\deps"
set "VCPKG_DIR=%DEPS_DIR%\vcpkg"
set "CMAKE_BUILD_DIR=%GLOBAL_BUILD_DIR%\target"
set "SOURCE_DIR=%BASE_DIR%"
set "LOG_FILE=%BASE_DIR%\build_log.txt"

:: Create directories
if not exist "%GLOBAL_BUILD_DIR%" mkdir "%GLOBAL_BUILD_DIR%"
if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

:: --- Checks ---
where cmake >nul 2>nul || (echo [ERROR] CMake not found. & goto :END)
where git >nul 2>nul || (echo [ERROR] Git not found. & goto :END)
where ninja >nul 2>nul || (echo [ERROR] Ninja not found. & goto :END)

:: --- Find Visual Studio ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)
if not exist "%VS_PATH%" (echo [ERROR] Visual Studio 2022 not found. & goto :END)

:: --- Setup Environment ---
echo [INFO] Setting up VC++ Environment...
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" || (echo [ERROR] vcvars64.bat failed. & goto :END)

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

:: --- CMake Configure & Build ---
echo [INFO] Configuring CMake and installing dependencies...
if not exist "%CMAKE_BUILD_DIR%" mkdir "%CMAKE_BUILD_DIR%"

:: Simple PowerShell wrapper for live log + error exit code
powershell -Command "& { cmake -S '%SOURCE_DIR%' -B '%CMAKE_BUILD_DIR%' -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE='%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake' -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_MANIFEST_DIR='%BASE_DIR%' -DEnable_GUI=ON -DSLIC3R_STATIC=OFF 2>&1 | Tee-Object -FilePath '%LOG_FILE%'; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake Configuration failed.
    goto :END
)

echo [INFO] Building Slic3r (Live Output)...
powershell -Command "& { cmake --build '%CMAKE_BUILD_DIR%' --parallel --verbose 2>&1 | Tee-Object -FilePath '%LOG_FILE%' -Append; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed.
    echo.
    echo ----- FOUND ERRORS IN LOG -----
    powershell -Command "Get-Content '%LOG_FILE%' | Where-Object { $_ -match ':\s+error\s+' -or $_ -match 'fatal\s+error' -or $_ -match '^FAILED:' } | ForEach-Object { Write-Host $_ }"
    echo -------------------------------
    goto :END
)

echo [INFO] Copying resources...
if not exist "%GLOBAL_BUILD_DIR%\var" mkdir "%GLOBAL_BUILD_DIR%\var"
xcopy /E /I /Y "%BASE_DIR%\var" "%GLOBAL_BUILD_DIR%\var" >nul

echo.
echo [SUCCESS] Build complete!
echo Binary: %CMAKE_BUILD_DIR%\slic3r.exe
echo.

:END
echo.
echo [INFO] Script finished.
pause