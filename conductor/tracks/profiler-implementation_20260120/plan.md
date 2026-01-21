# Implementation Plan: ElementProfiler Completion

## Phase 1: Foundation & Shared UI Elements [checkpoint: b8990a9]
- [x] Task: Integrate Icons & Fonts [bf0914d]
    - [x] Include `IconsMaterialSymbols.h` and ensure icon constants are usable.
    - [x] Implement/Verify `april::ui::getMonospaceFont()` availability.
- [x] Task: Implement V-Sync Placeholder [bf0914d]
    - [x] Add `isVsync()` and `setVsync(bool)` placeholder methods to `ElementProfiler`.
    - [x] Implement `draw_vsync_checkbox()` using these placeholders.
- [x] Task: Conductor - User Manual Verification 'Phase 1' (Protocol in workflow.md) [b8990a9]

## Phase 2: Data Handling & Table View
- [ ] Task: Data Collection Refinement
    - [ ] Ensure `update_data` and `add_entries` correctly map `april::core` snapshots to `EntryNode` trees.
- [ ] Task: Table View Completion
    - [ ] Implement `display_table_node` with full detailed mode support and monospace font usage.
    - [ ] Implement `render_table` with responsive grid support (side-by-side tables) and clipboard logging.
- [ ] Task: Conductor - User Manual Verification 'Phase 2' (Protocol in workflow.md)

## Phase 3: Graphical Visualizations (ImPlot)
- [ ] Task: Bar Chart Implementation
    - [ ] Implement `render_bar_chart` with stacked/horizontal support.
- [ ] Task: Pie Chart Implementation
    - [ ] Implement `render_pie_chart` and the recursive `render_pie_chart_node` for hierarchical rings.
- [ ] Task: Line Chart Implementation
    - [ ] Implement full `render_line_chart` including temporal smoothing for Y-axis and stacked GPU area fills.
    - [ ] Add the interactive tooltip for frame-specific inspection.
- [ ] Task: Conductor - User Manual Verification 'Phase 3' (Protocol in workflow.md)

## Phase 4: Persistence & Final Integration
- [ ] Task: Settings Handler Migration
    - [ ] Implement the `add_settings_handler` logic to save/load window visibility to `imgui.ini`.
- [ ] Task: Final Polish & Cleanup
    - [ ] Ensure all `onUIRender` logic correctly handles multiple views and default tab selection.
- [ ] Task: Conductor - User Manual Verification 'Phase 4' (Protocol in workflow.md)
