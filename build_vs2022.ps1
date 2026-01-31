<#
.SYNOPSIS
    Automated build script for Slic3r using Visual Studio 2022 and vcpkg.

.DESCRIPTION
    This script sets up a local vcpkg environment, installs necessary dependencies 
    (Boost, wxWidgets), and builds Slic3r using CMake and MSVC 2022.

.NOTES
    Requires: Git, CMake, Visual Studio 2022 (with C++ Desktop Development workload).
    Internet connection required for first run to download dependencies.
#>

$ErrorActionPreference = "Stop"

# --- Configuration ---
$VcpkgUrl = "https://github.com/microsoft/vcpkg.git"
$VcpkgDir = Join-Path $PSScriptRoot "vcpkg"
$BuildDir = Join-Path $PSScriptRoot "build-msvc"
$SourceDir = Join-Path $PSScriptRoot "src"

# Define dependencies
$Dependencies = @("boost-system", "boost-thread", "boost-filesystem", "boost-nowide", "wxwidgets")

# --- Checks ---
Write-Host "Checking prerequisites..." -ForegroundColor Cyan
if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
    Write-Error "CMake is not found in PATH. Please install CMake."
}
if (-not (Get-Command "git" -ErrorAction SilentlyContinue)) {
    Write-Error "Git is not found in PATH. Please install Git."
}

# --- Vcpkg Setup ---
if (-not (Test-Path $VcpkgDir)) {
    Write-Host "Cloning vcpkg to $VcpkgDir..." -ForegroundColor Cyan
    git clone $VcpkgUrl $VcpkgDir
}

if (-not (Test-Path "$VcpkgDir\vcpkg.exe")) {
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Cyan
    Push-Location $VcpkgDir
    .\bootstrap-vcpkg.bat
    Pop-Location
}

# --- Install Dependencies ---
Write-Host "Installing dependencies via vcpkg (this may take a while)..." -ForegroundColor Cyan
$VcpkgExe = Join-Path $VcpkgDir "vcpkg.exe"
# Install for x64-windows to match VS2022 default
& $VcpkgExe install --triplet x64-windows @Dependencies

# --- CMake Configure ---
Write-Host "Configuring CMake for Visual Studio 2022..." -ForegroundColor Cyan
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

$ToolchainFile = Join-Path $VcpkgDir "scripts/buildsystems/vcpkg.cmake"

# Using -S (Source) and -B (Build)
cmake -S $SourceDir -B $BuildDir -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="$ToolchainFile" `
      -DVCPKG_TARGET_TRIPLET=x64-windows `
      -DEnable_GUI=ON `
      -DSLIC3R_STATIC=ON

# --- Build ---
Write-Host "Building project (Release config)..." -ForegroundColor Cyan
cmake --build $BuildDir --config Release --parallel

Write-Host "Build complete!" -ForegroundColor Green
Write-Host "Executable should be in: $BuildDir\Release\slic3r.exe" -ForegroundColor Green
