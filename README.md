![](var/Slic3r_128px.png) Slic3r
======

Slic3r is a G-code generator for 3D printers. It converts 3D models (STL, OBJ, AMF, 3MF) into G-code instructions for FFF/FDM printers. Born in 2011 within the RepRap community, it has become a versatile toolpath generator, supporting a wide range of printers and acting as a testbed for new 3D printing technologies.

**Slic3r is:**
*   **Open Source:** Independent and community-driven.
*   **Compatible:** Supports all major G-code dialects (Marlin, Repetier, Klipper, etc.).
*   **Advanced:** Offers granular control over every aspect of the printing process.
*   **Modular:** Built on `libslic3r`, a powerful C++ library for geometry processing.

### <a name="features"></a>Features
*   **G-code generation** for FFF/FDM printers.
*   **Conversion** between STL, OBJ, AMF, 3MF, and POV formats.
*   **Auto-repair** of non-manifold meshes.
*   **SVG export** of slices.
*   **3D Preview** and toolpath visualization.
*   **Multiple Extruder** support.
*   **Mesh Cutting** and manipulation tools.

### Architecture & Language
The core of Slic3r is written in **C++17** for performance and robustness. The GUI is built using **wxWidgets**, and dependencies are managed via **vcpkg**.

### <a name="install"></a>Building from Source

Slic3r uses **CMake** as its build system and **vcpkg** for dependency management.

#### Windows (Gemini / Automated)
For a streamlined build experience on Windows, use the provided `build_gemini.bat` script. This script handles:
1.  Setting up the Visual Studio environment.
2.  Bootstrapping `vcpkg`.
3.  Installing all required dependencies (Boost, wxWidgets, etc.).
4.  Configuring and building Slic3r.

**Prerequisites:**
*   Visual Studio 2022 (with C++ Desktop Development workload).
*   Git.
*   Ninja (optional, but recommended for faster builds).

**Steps:**
```cmd
cd Slic3r-master
build_gemini.bat
```
The resulting executable will be in `build/target/Release/slic3r.exe`.

#### Manual Build (Cross-Platform)
If you prefer to manage the build yourself:

1.  **Install Dependencies:**
    Use `vcpkg` to install the required packages:
    ```bash
    vcpkg install boost-system boost-thread boost-filesystem boost-nowide boost-polygon boost-algorithm boost-asio boost-locale boost-property-tree boost-lexical-cast boost-any boost-bind boost-date-time boost-foreach wxwidgets
    ```

2.  **Configure:**
    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
    ```

3.  **Build:**
    ```bash
    cmake --build . --config Release
    ```

### Directory Structure

*   `src/`: Main C++ source code.
    *   `src/libslic3r/`: The core slicing engine (geometry, infill, G-code generation).
    *   `src/GUI/`: The wxWidgets-based graphical user interface.
    *   `src/test/`: Unit tests (using Catch2).
*   `utils/`: Helper scripts for maintenance and batch processing.
*   `var/`: Application assets (icons, images).
*   `package/`: Packaging scripts for different platforms.

### Dependencies
Slic3r relies on modern open-source libraries:
*   **Boost:** (System, Thread, Filesystem, Nowide, etc.)
*   **wxWidgets:** Cross-platform GUI toolkit.
*   **Eigen:** Linear algebra.
*   **Clipper:** Polygon clipping.
*   **Poly2Tri:** Triangulation.
*   **Miniz:** ZIP compression.

### Contributing
Contributions are welcome! Please read `CONTRIBUTING.md` (if available) and check the [GitHub Issues](https://github.com/slic3r/Slic3r/issues) for active tasks.

### Acknowledgements
Slic3r was originally created by Alessandro Ranellucci (@alranel). It is maintained by a community of developers.