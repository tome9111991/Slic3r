# PresetEditor & Configuration Dialogs - To-Do List

This document tracks the missing features, bugs, and desired improvements for the C++ port of the Preset Editor (`src/GUI/Dialogs/PresetEditor.*`).

## 1. Critical Functionality (High Priority)

### Dynamic Extruder Tabs (`PrinterEditor`)
- **Current Status:** Hardcoded "Extruder 1" page in `PrinterEditor::_build()`.
- **Goal:** Dynamically add/remove Extruder tabs based on the `extruders_count` setting.
- **Implementation:**
    - Listen for changes to `extruders_count`.
    - Rebuild the tree/pages when count changes.
    - Handle `extruder_offset` (vector) visualization.

### Bed Shape Editor
- **Current Status:** Missing. Comment in code: `// Bed shape button missing in C++ framework`.
- **Goal:** A graphical dialog to set bed size, origin, and shape (Rectangular/Circular/Custom).
- **Reference:** Original Perl `BedShapeDialog.pm`.

### Compatible Printers / Dependencies
- **Current Status:** Stubs (`reload_compatible_printers_widget`, `compatible_printers_widget`).
- **Goal:** 
    - Allow restricting a Print/Material preset to specific Printer presets.
    - Filter the "Preset" dropdowns in the Plater based on these compatibility rules.

### "Quick Settings" (Pinning)
- **Current Status:** Placeholder in `Plater`. No logic in `PresetEditor`.
- **Goal:** 
    - Add a "Pin" icon next to every option in `OptionsGroup`.
    - Persist pinned options in `slic3r.ini`.
    - Dynamically update the `Plater` Quick Settings panel when pins change.

## 2. UI/UX Improvements

### Search/Filter
- **Goal:** A search bar at the top of the Preset Editor.
- **Behavior:** Filtering should hide irrelevant tree nodes and highlight matching options in the right pane.

### Syntax Highlighting for G-Code
- **Current Status:** Plain `wxTextCtrl`.
- **Goal:** Use `wxStyledTextCtrl` (Scintilla) for G-Code fields (Start/End G-code) to provide basic coloring.

### Post-Processing Scripts
- **Current Status:** Simple text field.
- **Goal:** A structured list editor (Add/Remove/Move Up/Down) for script paths.

## 3. Architecture & Refactoring

### Data Binding
- **Current Status:** Uses `KillFocus` / `TextEnter` hacks in `save_preset` to force updates.
- **Goal:** Robust, real-time two-way data binding between `Config` and `UI_Field`.

### Dirty State Logic
- **Current Status:** Basic implementation.
- **Goal:** Rigorous testing of the "modified" flag (`*`). Ensure "Revert" functionality works 100%.

## 4. Theming (Dark Mode)
- **TreeCtrl:** Verify `wxTreeCtrl` appearance in Dark Mode on Windows.
- **Icons:** Ensure all referenced SVGs exist and are monochrome/recolorable.
