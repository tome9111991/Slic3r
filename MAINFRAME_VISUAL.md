# Slic3r MainFrame Technical Wireframe

This document visualizes the layout of the MainFrame application window using the technical variable names from the C++ source code (`src/GUI/MainFrame.cpp` and `src/GUI/Plater.cpp`).

## Technical Wireframe

```text
+-------------------------------------------------------------------------------+
| File | Plater | Object | Settings | View | Window | Help    (ThemedMenuBar)   |
+-------------------------------------------------------------------------------+
| [btn_prepare] [btn_preview] [btn_device]              [btn_slice] [btn_export]| <- top_bar (wxPanel)
+-------------------------------------------------------------------------------+
| [ Add ][ Del ][ Reset ][ Arrange ] | [ + ][ - ] | [ -45 ][ +45 ][Scale]...    | <- toolbar_panel
+-----------------------+-------------------------------------------------------+
|  sidebar_content      |                                                       |
|  (ThemedPanel)        |            preview_notebook (wxSimplebook)            |
|  [Hard Black Border]  |                                                       |
|                       |  +-------------------------------------------------+  |
|  +-----------------+  |  |                                                 |  |
|  | _presets        |  |  |           canvas3D (Plate3D)                    |  |
|  | (PresetChooser) |  |  |                                                 |  |
|  +-----------------+  |  |                  OR                             |  |
|  |                 |  |  |                                                 |  |
|  +-----------------+  |  |           preview3D (Preview3D)                 |  |
|  | quick_settings  |  |  |                                                 |  |
|  | _section        |  |  |                                                 |  |
|  | (ThemedSection) |  |  |                                                 |  |
|  +-----------------+  |  +-------------------------------------------------+  |
|  |                 |  |                                                       |
|  +-----------------+  |                                                       |
|  | object_info     |  |                                                       |
|  | _section        |  |                                                       |
|  | (ThemedSection) |  |                                                       |
|  +-----------------+  |                                                       |
+-----------------------+-------------------------------------------------------+
```

## Component Mapping & Notes

### 1. Main Navigation (`MainFrame.cpp`)
*   **`ThemedMenuBar`**: Custom owner-drawn menu bar at the very top. Replaces native `wxMenuBar` for full Dark Mode / ThemeManager consistency.
*   **`top_bar`** (`wxPanel`): The container for the main application header.
*   **`btn_prepare` / `btn_preview` / `btn_device`** (`FlatToggleButton`): Tabs controlling the main `tabpanel` selection.
*   **`btn_slice` / `btn_export`** (`OvalButton`): Primary action buttons.

### 2. The Plater View (`Plater.cpp`)
The `Plater` is the main page of the `tabpanel`. Its layout is a horizontal split:

*   **`sidebar_content` (The Sidebar)**: 
    *   **Type:** `ThemedPanel`
    *   **Style:** Hard Black Border (`SetBorder(true, *wxBLACK)`)
    *   **Layout:** Contains `left_sizer` (`wxBoxSizer(wxVERTICAL)`).
    *   **Children:**
        1.  `_presets` (`PresetChooser`): Dropdowns for Printer/Material/Process.
        2.  `quick_settings_section` (`ThemedSection`): Collapsible section for pinned settings.
        3.  `object_info_section` (`ThemedSection`): Collapsible section for Object copies, size, etc.

*   **`preview_notebook` (The Canvas)**:
    *   The main workspace on the **RIGHT**.
    *   Toggles between `canvas3D` (Editor) and `preview3D` (G-Code Preview).

### 3. Global Elements

*   **`tabpanel`**: A `wxSimplebook` in `MainFrame` that holds the `Plater` and `Controller` pages.

*   **`toolbar_panel`**: 
    *   **Class:** `wxPanel` containing a `wxBoxSizer` (Horizontal).
    *   **Child Elements:** `ToolbarButton` (Custom `wxPanel` with icons).
    *   **Tools (IDs):**
        *   *Management:* `TB_ADD`, `TB_REMOVE`, `TB_RESET`, `TB_ARRANGE`
        *   *Copies:* `TB_MORE`, `TB_FEWER`
        *   *Manipulation:* `TB_45CCW`, `TB_45CW`, `TB_SCALE`, `TB_SPLIT`, `TB_CUT`
        *   *Settings:* `TB_SETTINGS`, `TB_LAYERS`
