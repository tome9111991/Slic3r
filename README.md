# Slic3r (Modern C++ Port)

This project is an active, work-in-progress effort to port the original Perl-based Slic3r application to a pure **Modern C++ (C++23)** codebase. The goal is to create a high-performance, maintainable, and self-contained executable.

> **⚠️ Status:** Active Development / Alpha.
> While the core slicing engine is largely functional, many UI features and advanced tools are still being implemented or optimized. This is **not** yet a 1:1 replacement for the legacy Perl version.

## 🚀 Project Status

### Core Engine (`libslic3r`)
The slicing logic (geometry, infill, G-code generation) has been ported to C++, but active work continues on:
*   **Optimization:** Many algorithms are marked for performance improvements (e.g., infill generation, support material).
*   **Refactoring:** Cleaning up legacy logic and unifying data structures.
*   **Feature Parity:** Some specific slicing features (e.g., complex cuts, specific support generation edge cases) may still be incomplete or experimental.

### GUI (`src/GUI`)
The user interface is being rebuilt from scratch using **wxWidgets** and **ImGui**, featuring a new custom theming system.
*   **Implemented:** Main application shell, 3D Plater/Preview, basic object manipulation, G-code export.
*   **In Progress:**
    *   **Preset Management:** Configuration loading/saving is partially implemented but needs a centralized manager.
    *   **Advanced Tools:** Features like "Quick Slice", object cutting/splitting, and the configuration wizard are currently stubs or WIP.
    *   **Theming:** Custom "owner-drawn" controls for full Dark/Light mode support on Windows.

## 🛠 Tech Stack
*   **Language:** C++23
*   **Build System:** CMake + Ninja / Visual Studio 2022
*   **Dependency Management:** `vcpkg` (Boost, wxWidgets, OpenSSL, etc.)
*   **GUI Library:** wxWidgets 3.2+ (Core UI) + ImGui (3D Overlays)
*   **Rendering:** Modern OpenGL 4.6 (Glad + GLM)

## 📂 Directory Structure

| Directory | Description |
| :--- | :--- |
| **`src/`** | **Active C++ Source.** |
| ├─ `libslic3r/` | Core slicing engine. |
| └─ `GUI/` | User interface and rendering logic. |
| **`port/`** | **Legacy Reference.** Contains the original Perl/XS code for comparison. |
| **`resources/`** | Assets (Icons, Shaders, Fonts). |

## 🏗 Building from Source

### Prerequisites (Windows)
*   **Visual Studio 2022** (Desktop C++ Workload)
*   **CMake** (3.21 or later)
*   **PowerShell 5.1+**

### Build Instructions
The project uses a helper script to manage `vcpkg` dependencies automatically.

1.  **Clone the repository:**
    ```powershell
    git clone <repo-url>
    cd Slic3r
    ```
2.  **Run the build script:**
    ```powershell
    .\build_vs2022.bat
    ```
    *   This will bootstrap `vcpkg`, install dependencies (Boost, wxWidgets, etc.), and generate the Visual Studio solution in `build/`.
    *   The compiled executable will be at `build/target/Debug/slic3r.exe` (or Release).

### Manual CMake Build
If you prefer running CMake directly (ensure `VCPKG_ROOT` is set):
```bash
mkdir build; cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build . --config Release
```

## 🤝 Contributing
Contributions are welcome!
*   **Search for TODOs:** The codebase contains many `// TODO` and `// FIXME` comments indicating areas that need attention.
*   **Check `GUI_TODO.md`:** (Note: May be partially outdated) for UI-specific tasks.
*   **Legacy Comparison:** When implementing a feature, refer to the Perl implementation in `port/` to ensure logic parity.

## ⚖️ License
Licensed under **AGPLv3**. Based on the original Slic3r by Alessandro Ranellucci.