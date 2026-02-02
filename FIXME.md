# Visual Artifact Investigation: Top-Left Dark Square

## Status
**Persistent.** A small dark square artifact remains visible at position (0,0) of the `Plater` panel (top-left, just below the modern navigation bar).

## Attempted Fixes (Result: Failed)
1. **Centering Toolbar Buttons**: Centering the buttons exposed the artifact on the far left.
2. **Margin Test**: Added a 20px margin to the `toolbar_panel`. The artifact remained at (0,0), proving it is NOT a child of the `toolbar_panel` but a child of the `Plater` itself.
3. **Debug Coloring**: 
   - Colored `Plater` Red: Artifact appeared as a dark square on the Red background.
   - Colored `toolbar_panel` Red: Artifact was separate from the panel.
4. **Hiding Suspects**: 
   - Manually hidden `PresetChooser` and `preview_notebook`. Artifact remained.
   - Commented out the "Info" box (`wxStaticBox`). Artifact remained.
   - Removed the vertical separator `wxStaticLine`. Artifact remained.
5. **Brute Force Hiding**: 
   - Iterated through all children of `Plater` at the end of the constructor and called `Hide()` on anything that wasn't a known managed component. **The artifact persisted**, suggesting it might not be a standard `wxWindow` child or it's being re-drawn/re-created after the constructor.
6. **Flag Removal**: 
   - Removed `wxTAB_TRAVERSAL` from `Plater` constructor. Artifact remained.

## Theories for Future Investigation
- **Native OS Artifact**: Could be a native Win32 artifact from a `wxToolBar` that was previously attached via `SetToolBar` or similar legacy code in `MainFrame` that hasn't been fully cleaned.
- **Initialization Order**: A widget might be created with `NULL` or `this` parent and then moved later, but a "ghost" remains at (0,0) due to rendering bugs in the custom dark theme.
- **Owner-Drawn remnants**: The port might contain legacy drawing code (e.g. `EVT_PAINT`) in a base class that draws a placeholder square.

## Impact
Purely visual. Does not affect functionality. Partially obscured when toolbar buttons are left-aligned.
