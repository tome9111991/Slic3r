# Slic3r Modernization Roadmap

This roadmap documents the process of porting Slic3r (Legacy) to a modern C++ tech stack (VS2022, vcpkg, C++17/20) and removing outdated dependencies.

## 🟢 Phase 1: Build System & Basic Repairs (Completed)
- [x] **Transition Build System**: Migrated to modern CMake with vcpkg integration.
- [x] **Un-vendoring (Part 1)**: Removed local `boost` copies (`xs/src/boost`) in favor of vcpkg `boost` (1.90.0).
- [x] **Fix Linker Errors**: Resolved `boost::nowide` symbol conflicts (Static vs. Dynamic / Namespace `detail` vs `details`).
- [x] **Runtime Fixes**: Fixed crashes caused by stricter wxWidgets 3.3 assertions (`SetContainingWindow`).
- [x] **Resources**: Automated copying of the `var` directory in the build scripts.

## 🟡 Phase 2: "Un-vendoring" Remaining Libraries (High Priority)
The `src` and `xs/src` directories still contain bundled source code for libraries that should be managed by vcpkg. Removing these makes the build cleaner and safer.

- [ ] **Clipper**: Currently compiled from `xs/src/clipper.cpp`.
    - *Action*: `vcpkg install clipper`, remove local source from `CMakeLists.txt`, link against package.
- [ ] **Admesh**: Currently compiled from `xs/src/admesh/*.c`.
    - *Action*: `vcpkg install admesh`, update `CMakeLists.txt`.
- [ ] **Poly2Tri**: Currently compiled from `xs/src/poly2tri`.
    - *Action*: `vcpkg install poly2tri`, update `CMakeLists.txt`.
- [ ] **Expat**: Currently compiled from `xs/src/expat`.
    - *Action*: `vcpkg install expat`, update `CMakeLists.txt`.
- [ ] **Eigen**: Check usage. Slic3r needs robust geometry math; vcpkg provides the latest stable Eigen3.

## 🟠 Phase 3: Perl Integration & Legacy Logic
Legacy Slic3r is a hybrid of Perl (Logic/GUI wrapper) and C++ (Core/Geometry). Currently, we have only built the pure C++ standalone version (`slic3r.exe`). The Perl bindings (`XS`) are the "Glue".

- [ ] **Verify XS Compilation**: Check if `perl Build.PL` works with the new VS2022 compilers and vcpkg paths.
- [ ] **Avoid DLL Hell**: Ensure the Perl environment (e.g., Strawberry Perl) and compiled C++ DLLs are compatible (avoiding CRT mismatches).
- [ ] **Run Tests**: Execute the Perl test suite (`prove`) to verify logical integrity.

## 🔴 Phase 4: GUI & Rendering Modernization
Slic3r uses outdated techniques for its display.

- [ ] **OpenGL**: The current code uses "Immediate Mode" (`glBegin`/`glEnd`), which is deprecated and slow.
    - [ ] Migrate to Modern OpenGL (VBOs/VAOs) for better performance on new GPUs.
- [ ] **wxWidgets 3.3 Adjustments**:
    - [ ] **High DPI Support**: Ensure icons and text scale correctly on 4K monitors (consider SVG icons instead of PNGs).
    - [ ] **Dark Mode**: Enable Windows 11 Dark Mode support via wxWidgets.

## 🔵 Phase 5: Housekeeping & DevOps
Cleaning up the artifacts of the last decade.

- [ ] **Delete Legacy CI**: Remove `.travis.yml` and `appveyor.yml` (they are broken and unused).
- [ ] **Update Documentation**: Rewrite `README.md` to reflect the new build process (VS2022 + vcpkg) instead of the old manual instructions.
- [ ] **GitHub Actions**: Create a `.github/workflows/build_windows.yml` to build the project automatically on push.
- [ ] **Installer**: Create a WiX or NSIS script to produce an installable `.msi` or `.exe`.

---

## Technical Notes
*   **Vcpkg**: All new libraries must be added to `vcpkg.json`.
*   **CMake Convention**: Do not include manual `.cpp` files from libraries in `CMakeLists.txt` anymore. Always use `find_package()` and `target_link_libraries()`.
