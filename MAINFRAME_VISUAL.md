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
|  left_sizer (Panel)   |                                                       |
|                       |            preview_notebook (wxNotebook/Simplebook)   |
|                       |                                                       |
|  +-----------------+  |  +-------------------------------------------------+  |
|  | _presets        |  |  |                                                 |  |
|  | (PresetChoice)  |  |  |                                                 |  |
|  +-----------------+  |  |                                                 |  |
|                       |  |                                                 |  |
|  +-----------------+  |  |           canvas3D (Plate3D)                    |  |
|  |object_info_sizer|  |  |                  OR                             |  |
|  |                 |  |  |           preview3D (Preview3D)                 |  |
|  | .choice         |  |  |                                                 |  |
|  | .copies         |  |  |                                                 |  |
|  | .size ...       |  |  |                                                 |  |
|  +-----------------+  |  +-------------------------------------------------+  |
|                       |                                                       |
+-----------------------+-------------------------------------------------------+
```

## Component Mapping & Notes

### 1. Main Navigation (`MainFrame.cpp`)
*   **`ThemedMenuBar`**: Custom owner-drawn menu bar at the very top (File, Plater, Object...). Replaces the native `wxMenuBar` for full Dark Mode / ThemeManager compatibility.
*   **`top_bar`**: The container for the main application header, below the menu bar.
*   **`btn_prepare` / `btn_preview` / `btn_device`**: `FlatToggleButton` instances that control the main `tabpanel` selection.
*   **`btn_slice` / `btn_export`**: `OvalButton` instances for primary actions.

### 2. The Plater View (`Plater.cpp`)
The `Plater` is the main page of the `tabpanel`. Its layout is a horizontal split:

*   **`right_sizer` (The Sidebar)**: 
    *   **WARNING:** Despite the name, this sizer is added **FIRST** to the horizontal sizer, which places it on the **LEFT** side of the screen in the current implementation.
    *   Contains: `_presets` (Preset selectors) and `object_info_sizer` (Object list and transformation info).
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
