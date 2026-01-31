# Slic3r Project Context

## Project Overview
Slic3r is a G-code generator for 3D printers. It converts 3D models (STL, OBJ, AMF) into printing instructions (G-code). 
The project features a hybrid architecture:
- **Core Logic:** Written in C++ (C++11/14) for performance, located in `src/` and `xs/src/libslic3r`.
- **Scripting/GUI:** Historically written in Perl, with bindings to the C++ core via XS (Extension Subroutines).
- **GUI:** Transitioning to C++ (wxWidgets).

## Environment & Prerequisites (Windows)
The development environment on Windows relies heavily on **Strawberry Perl**, which provides the Perl runtime and the **MinGW** compiler environment.

### Required Tools
- **Strawberry Perl:** (e.g., v5.24 as seen in CI configs)
- **CMake:** For building the C++ parts.
- **Git:** For version control.
- **MinGW-w64:** Usually bundled with Strawberry Perl.

### Dependencies
- **Boost:** (Requires explicit path configuration)
- **wxWidgets:** For the GUI.
- **FreeGLUT:** Used for some graphics rendering.
- **Perl Modules:** Various CPAN modules (handled via `cpanm` or `Build.PL`).

## Building & Running

### Automated Build (Reference)
The project uses AppVeyor for Windows CI. The scripts in `package/win/` are the most reliable reference for the build process:
- `package/win/appveyor_preinstall.ps1`: Installs dependencies (Boost, wxWidgets, Strawberry Perl).
- `package/win/appveyor_buildscript.ps1`: Configures environment and runs the build.

### Manual Build Steps
1.  **Install Dependencies:** Ensure Boost and wxWidgets are available.
2.  **Configure Environment:** Set variables like `BOOST_DIR`, `WXDIR` to your installation paths.
3.  **Build:**
    ```powershell
    # Install Perl dependencies
    cpanm --installdeps .
    
    # Build the XS extensions (C++ core bindings)
    perl Build.PL
    
    # Run tests
    perl Build.PL test
    ```
    *Note: The root `Build.PL` orchestrates the build process, delegating to `xs/Build.PL` for the C++ components.*

## Directory Structure

| Directory | Description |
| :--- | :--- |
| **`src/`** | Main C++ source code for the executable and GUI. |
| **`xs/`** | Perl XS bindings. `xs/src/libslic3r` contains the C++ core logic. |
| **`lib/`** | Perl modules (`.pm`), defining the scripting logic and legacy UI parts. |
| **`t/`** | Test suite (Perl). |
| **`package/`** | Packaging and build scripts for various platforms (Windows, Linux, OSX). |
| **`utils/`** | Utility scripts (Perl/Shell) for maintenance and data processing. |
| **`var/`** | Assets like icons and images. |

## Development Conventions
- **Hybrid Codebase:** Changes often span both C++ (logic) and Perl (interface/glue).
- **Testing:** Uses standard Perl test harnesses (`t/` directory). Run via `prove` or `perl Build.PL test`.
- **Platform Specifics:** Windows builds require careful handling of paths and compiler compatibility (MinGW vs MSVC).
