# Slic3r C++ Porting Project

## Project Overview
This project is an active effort to port the original Perl-based Slic3r application to a pure **Modern C++ (C++17)** application.
The goal is to eliminate the Perl dependency entirely, resulting in a faster, more maintainable, and self-contained executable.

*   **Current Status:** Core slicing engine (`libslic3r`) is fully ported. The GUI is approximately 50% complete.
*   **Active Development:** `src/` (C++).
*   **Legacy Reference:** `port/` (Perl/XS code moved here for reference).

## Porting Strategy
The project follows a strict porting roadmap defined in `port/PORTING_PLAN.md`.
The legacy Perl code in `port/lib/` and `port/xs/` serves as the "source of truth" for logic that needs to be reimplemented in `src/`.

**Key Gaps (To-Do):**
1.  **Preset Management:** Saving/Loading config profiles (`.ini`).
2.  **Configuration Wizard:** First-run setup logic.
3.  **Undo/Redo:** History stack for the Plater.
4.  **2D/3D Interaction:** Advanced object manipulation (Cut, Mirror, etc.).

## Directory Structure

| Directory | Description |
| :--- | :--- |
| **`src/`** | **ACTIVE** C++ source code. Contains `libslic3r` (core) and `GUI` (wxWidgets). |
| **`port/`** | **LEGACY** Perl/XS code. Use this for reference when reimplementing features. |
| **`port/lib/`** | Legacy Perl modules (GUI logic often found here). |
| **`port/xs/`** | Legacy XS bindings. |
| **`build/`** | Build artifacts. |
| **`package/`** | Packaging scripts. |
| **`var/`** | Application resources (icons, models). |

## Building & Running

### Prerequisites (Windows)
- Visual Studio 2022
- PowerShell 7 or 5.1
- CMake & Ninja (installed via VS or separately)
- **Boost 1.90.0**
- **wxWidgets 3.3.1**

### Build Command
Use the provided batch script to build the C++ version:
```powershell
.\build_vs2022.bat
```
This script handles `vcpkg` dependencies and CMake configuration automatically.

*   **Executable:** `build\target\slic3r.exe`

## Development Guidelines
1.  **Read Legacy:** When tasked to "fix" or "implement" a feature, first locate the corresponding Perl implementation in `port/lib/Slic3r/`.
2.  **Implement C++:** Reimplement the logic in `src/` using C++17 and wxWidgets.
3.  **Verify:** Ensure the C++ behavior matches the legacy Perl behavior.
4.  **Style:** Follow the surrounding C++ code style (indentation, naming).
5.  **Forward Thinking:** When implementing new features, always design with future scalability and modularity in mind. Don't just port the logic; consider how it can be improved or extended in the modern C++ context.

## Useful References
- **Perl GUI:** `port/lib/Slic3r/GUI/`
- **C++ GUI:** `src/GUI/`