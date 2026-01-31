# Slic3r (Modern C++ Port)

Slic3r is a G-code generator for 3D printers. This is a modernized fork of the original Slic3r, refactored to be a pure C++ application, removing all Perl legacy code and dependencies. The architecture is now aligned with modern standards (similar to OrcaSlicer/PrusaSlicer), making it a robust foundation for future development.

## Features
*   **Pure C++:** No Perl interpreter required. High performance and easier to maintain.
*   **Modern Architecture:** flattened `src/` structure, CMake-based build system.
*   **G-code generation** for FFF/FDM printers.
*   **3D Preview** and toolpath visualization.
*   **Modular Core:** `libslic3r` is now a first-class citizen in the source tree.

## Directory Structure

*   `src/`: Main C++ source code.
    *   `src/libslic3r/`: The core slicing engine.
    *   `src/GUI/`: The wxWidgets-based user interface.
    *   `src/slic3r.cpp`: Application entry point.
*   `var/`: Application assets (icons, images).
*   `package/`: Packaging scripts.

## Building from Source

This project uses **CMake** as its build system.

### Windows
You can use the provided helper script:
```cmd
build_vs2022.bat
```

Or build manually using CMake and Visual Studio 2019/2022:

```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

**Requirements:**
*   Visual Studio 2022 (C++ Desktop Development)
*   CMake
*   Boost
*   wxWidgets

### Linux / macOS
**Note:** The current modernization effort has been focused and tested on Windows. While the project uses standard CMake and cross-platform libraries (Boost, wxWidgets), it may require adjustments to `CMakeLists.txt` or dependencies to build successfully on Linux or macOS.

```bash
mkdir build && cd build
cmake ..
make
```

## Credits
Based on Slic3r by Alessandro Ranellucci. Refactored for modern C++ environments.
