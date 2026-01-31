# Slic3r Project Context

## Project Overview
Slic3r is a G-code generator for 3D printers, converting 3D models (STL, OBJ, AMF, 3MF) into printing instructions.
This codebase represents the **Slic3r** project, featuring a hybrid architecture:
- **Core Logic & GUI:** Modern C++ (C++17) using wxWidgets, located in `src/`.
- **Scripting/Legacy:** Perl with XS bindings (`xs/` directory) and legacy UI components (`lib/`).

## Environment & Prerequisites (Windows)
- **OS:** Windows 11
- **Shell:** PowerShell 7 (`pwsh`) or 5.1.
- **Compilers:** Visual Studio 2022 (MSVC).
- **Build Tools:** CMake, Ninja.
- **Dependencies:** Managed via **vcpkg** (manifest mode).

## Building & Running

### Automated Build (Recommended)
The project uses `build_vs2022.bat` as the primary build script. This script handles:
1.  Visual Studio 2022 environment setup (`vcvars64.bat`).
2.  **vcpkg** setup (cloning & bootstrapping if missing).
3.  Dependency installation and CMake configuration.
4.  Compilation (Release mode).

```powershell
.\build_vs2022.bat
```
*   **Logs:** `build_log.txt`
*   **Output:** `build/target/slic3r.exe`

### Manual Build (CMake)
If you prefer to run CMake manually (e.g., for IDE integration):

1.  **Configure:**
    ```powershell
    cmake -S src -B build/target -G "Ninja" `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_TOOLCHAIN_FILE="build/deps/vcpkg/scripts/buildsystems/vcpkg.cmake" `
        -DVCPKG_TARGET_TRIPLET=x64-windows `
        -DVCPKG_MANIFEST_DIR="." `
        -DEnable_GUI=ON `
        -DSLIC3R_STATIC=OFF
    ```
2.  **Build:**
    ```powershell
    cmake --build build/target --parallel
    ```

### Running
- **C++ Executable:** `.\build\target\slic3r.exe`
- **Perl Wrapper:** `perl slic3r.pl` (Requires Perl environment setup, usually for legacy or headless use).

## Directory Structure

| Directory | Description |
| :--- | :--- |
| **`src/`** | Main C++ source code (Core logic + GUI). |
| **`xs/`** | Perl XS bindings connecting C++ core to Perl. |
| **`lib/`** | Perl modules (`.pm`). |
| **`t/`** | Test suite (Perl-based). |
| **`package/`** | Packaging scripts (AppVeyor, Travis, etc.). |
| **`utils/`** | Helper scripts (e.g., `amf-to-stl.pl`). |
| **`var/`** | Assets (icons, images, resources). |
| **`build/`** | Build artifacts (created by `build_auto.bat`). |

## Development Conventions
- **Language:** C++17 for new development; Perl for existing scripting/tests.
- **Style:** Adhere to existing indentation (spaces vs tabs) in the file being edited.
- **Testing:** 
    -   C++ Tests: `ctest` in the build directory.
    -   Perl Tests: `prove t/` (requires full Perl dev environment).
