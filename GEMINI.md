# Slic3r C++ Porting Project

## Project Overview
This project is an active effort to port the original Perl-based Slic3r application to a pure **Modern C++ (C++17)** application.
The goal is to eliminate the Perl dependency entirely, resulting in a faster, more maintainable, and self-contained executable.

*   **Current Status:** Core slicing engine (`libslic3r`) is fully ported. The GUI is approximately 50% complete.
    *   **Completed:** Build system migration (CMake/vcpkg), Basic Theme Infrastructure (Fonts, HiDPI, SVG).
    *   **Active:** Phase 2 "Un-vendoring" of external libraries (Clipper, Admesh, Poly2Tri).
*   **Active Development:** `src/` (C++).
*   **Legacy Reference:** `port/` (Perl/XS code moved here for reference).

## Porting Strategy
The project follows a strict porting roadmap defined in `port/PORTING_PLAN.md`.
The legacy Perl code in `port/lib/` and `port/xs/` serves as the "source of truth" for logic that needs to be reimplemented in `src/`.

**Key Gaps (To-Do):**
1.  **Un-vendoring:** Replace local sources in `src/` (Clipper, Admesh, Poly2Tri) with vcpkg packages.
2.  **Preset Management:** Saving/Loading config profiles (`.ini`).
3.  **Font Manager:** Centralized handling of UI and 3D Canvas fonts, including support for bundled TTF/OTF resources for cross-platform consistency.
4.  **Configuration Wizard:** First-run setup logic.
5.  **Undo/Redo:** History stack for the Plater.
6.  **2D/3D Interaction:** Advanced object manipulation (Cut, Mirror, etc.).

## Directory Structure

| Directory | Description |
| :--- | :--- |
| **`src/`** | **ACTIVE** C++ source code. Contains `libslic3r` (core) and `GUI` (wxWidgets). |
| **`port/`** | **LEGACY** Perl/XS code. Use this for reference when reimplementing features. |
| **`port/lib/`** | Legacy Perl modules (GUI logic often found here). |
| **`port/xs/`** | Legacy XS bindings. |
| **`build/`** | Build artifacts. |
| **`package/`** | Packaging scripts. |
| **`resources/`** | Application resources (icons, models). |

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
6.  **Comments:** Always include concise comments explaining the logic and intent behind the code to ensure maintainability and clarity during the porting process.

## GUI Development & Theming
To ensure full control over Light/Dark modes independent of system settings, we use a custom "Owner-Drawn" approach for UI controls.

### 1. New Widgets
*   **Location:** `src/GUI/Widgets/`
*   **Implementation:** Inherit from `wxControl` or `wxPanel`.
*   **Painting:** Override `OnPaint` (using `wxAutoBufferedPaintDC`) and draw manually using `wxGraphicsContext`.
*   **Events:** Emit standard wxWidgets events (e.g., `wxEVT_BUTTON`, `wxEVT_CHECKBOX`) so usage remains standard.
*   **Do NOT** use native controls (like standard `wxButton` or `wxCheckBox`) if they don't support full recoloring on Windows.

### 2. Theming Architecture
We enforce a strict separation between Application UI and 3D Canvas rendering:

*   **UI Elements (Windows, Panels, Buttons, Text):**
    *   **Manager:** `src/GUI/Theme/ThemeManager.hpp`
    *   **Usage:** Use `ThemeManager::GetColors()` to access `bg`, `surface`, `text`, `accent`, `header`.
    *   **Dark Mode Check:** `ThemeManager::IsDark()`.
    *   **Icons:** `ThemeManager::GetSVG("name", size, color)`.
    *   **Recoloring:** The manager replaces `#333` with `color` automatically. Use generic icons in `resources/images/`.

*   **Font Management (UI & 3D):**
    *   **Goal:** Provide a consistent typographic experience.
    *   **Manager:** Integrated into `ThemeManager` (for now) or a dedicated `FontManager`.
    *   **Usage:** `ThemeManager::GetFont(FontSize::Small, FontWeight::Normal)`.
    *   **Future:** Support loading embedded font resources (e.g., `resources/fonts/`) to avoid reliance on varying system fonts.

*   **3D Canvas (Plater, Preview, Toolpaths):**
    *   **Manager:** `src/GUI/Theme/CanvasTheme.hpp`
    *   **Usage:** Use `CanvasTheme::GetColors()` to access `bed_color`, `grid_color`, `canvas_bg_top`.
    *   **Note:** Only use this for OpenGL rendering contexts.

### 3. Workflow for New Elements
1.  Define the control in `src/GUI/Widgets/ThemedControls.hpp` (or a new file if complex).
2.  Implement drawing logic in `.cpp`, referencing `ThemeManager` for all colors.
3.  Add the source file to `CMakeLists.txt` (if new file created).
4.  Verify appearance in both Light (`ThemeManager::SetDarkMode(false)`) and Dark modes.

## Useful References
- **Perl GUI:** `port/lib/Slic3r/GUI/`
- **C++ GUI:** `src/GUI/`
