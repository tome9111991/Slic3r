@echo off
setlocal enabledelayedexpansion
set "EXIT_CODE=0"

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
set "CMAKE_BUILD_DIR=%GLOBAL_BUILD_DIR%\obj"
set "TARGET_DIR=%GLOBAL_BUILD_DIR%\target"
set "SOURCE_DIR=%BASE_DIR%"
set "LOG_FILE=%BASE_DIR%\build_log.txt"

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

:: --- Checks ---
where cmake >nul 2>nul || (echo %RED%[ERROR]%RESET% CMake not found. & set "EXIT_CODE=1" & goto :END)
where git >nul 2>nul || (echo %RED%[ERROR]%RESET% Git not found. & set "EXIT_CODE=1" & goto :END)
where ninja >nul 2>nul || (echo %RED%[ERROR]%RESET% Ninja not found. & set "EXIT_CODE=1" & goto :END)

:: --- Find Visual Studio ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)
if not exist "%VS_PATH%" (echo %RED%[ERROR]%RESET% Visual Studio 2022 not found. & set "EXIT_CODE=1" & goto :END)

:: --- Setup Environment ---
echo %YELLOW%[INFO]%RESET% Setting up VC++ Environment...
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" || (echo %RED%[ERROR]%RESET% vcvars64.bat failed. & set "EXIT_CODE=1" & goto :END)

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

:: --- CMake Configure & Build ---
echo %YELLOW%[INFO]%RESET% Configuring CMake and installing dependencies...
if not exist "%CMAKE_BUILD_DIR%" mkdir "%CMAKE_BUILD_DIR%"

:: Simple PowerShell wrapper for live log + error exit code
powershell -Command "& { cmake -S '%SOURCE_DIR%' -B '%CMAKE_BUILD_DIR%' -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE='%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake' -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_MANIFEST_DIR='%BASE_DIR%' -DEnable_GUI=ON -DSLIC3R_STATIC=OFF 2>&1 | Tee-Object -FilePath '%LOG_FILE%'; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }"

if %errorlevel% neq 0 (
    echo.
    echo %RED%[ERROR]%RESET% CMake Configuration failed.
    set "EXIT_CODE=1"
    goto :END
)

echo %YELLOW%[INFO]%RESET% Building Slic3r (Live Output)...
powershell -Command "& { cmake --build '%CMAKE_BUILD_DIR%' --parallel --verbose 2>&1 | Tee-Object -FilePath '%LOG_FILE%' -Append; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }"

if %errorlevel% neq 0 (
    echo.
    echo %RED%[ERROR]%RESET% Build failed.
    echo.
    echo %YELLOW%----- FOUND ERRORS IN LOG -----%RESET%
    powershell -Command "Get-Content '%LOG_FILE%' | Where-Object { $_ -match ':\s+error\s+' -or $_ -match 'fatal\s+error' -or $_ -match '^FAILED:' } | ForEach-Object { Write-Host $_ -ForegroundColor Red }"
    echo %YELLOW%-------------------------------%RESET%
    set "EXIT_CODE=1"
    goto :END
)

echo %YELLOW%[INFO]%RESET% Installing artifacts to target...
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
copy "%CMAKE_BUILD_DIR%\slic3r.exe" "%TARGET_DIR%\" >nul
if exist "%CMAKE_BUILD_DIR%\*.dll" copy "%CMAKE_BUILD_DIR%\*.dll" "%TARGET_DIR%\" >nul

echo %YELLOW%[INFO]%RESET% Copying resources...
if not exist "%TARGET_DIR%\resources" mkdir "%TARGET_DIR%\resources"
xcopy /E /I /Y "%BASE_DIR%\resources" "%TARGET_DIR%\resources" >nul

echo.
echo %GREEN%[SUCCESS]%RESET% Build complete!
echo Binary: %TARGET_DIR%\slic3r.exe
echo.


:END
echo.
echo %CYAN%[INFO]%RESET% Script finished.
if "%GITHUB_ACTIONS%"=="" pause
exit /b %EXIT_CODE%
