# Specification: ElementProfiler Implementation

## Overview
Migrate and complete the implementation of the `ElementProfiler` UI component from the `nvapp` framework to the April Engine's `april::ui` namespace. This component provides a comprehensive visualization of engine performance data (CPU/GPU timings) using Dear ImGui and ImPlot.

## Functional Requirements
- **Multiple Views:** Support adding and managing multiple independent profiler windows via `addView`.
- **Table View:**
    - Hierarchical display of CPU and GPU timings.
    - "Detailed" mode toggle (showing Avg, Last, Min, Max).
    - "Copy to Clipboard" functionality for performance data.
    - Responsive grid mode when multiple root nodes are present.
- **Bar Chart:**
    - Horizontal bar chart visualization.
    - Support for "Stacked" mode.
    - Categorized by root profiler nodes.
- **Pie Chart:**
    - Hierarchical ring visualization of timings.
    - Configurable depth (levels) and "CPU Total" vs "GPU Sum" modes.
- **Line Chart:**
    - Temporal visualization of CPU and GPU timings.
    - Stacked GPU areas with fill alpha.
    - Temporal smoothing for Y-axis scaling.
    - Tooltip display for specific frame data.
- **Persistence:** Implement an ImGui settings handler to save/load window visibility state to `imgui.ini`.
- **V-Sync Toggle:** Implement a placeholder UI toggle for V-Sync (hooking into a placeholder `setVsync` function).

## Non-Functional Requirements
- **C++23 Standards:** Follow April Engine's coding style (e.g., `auto` return types, snake_case for private methods, etc.).
- **Performance:** Ensure UI rendering and data collection are efficient and thread-safe (using `getSnapshots`).
- **Visuals:** Use Material Design icons and monospace fonts for numerical data (assuming `IconsMaterialSymbols.h` and `april::ui::getMonospaceFont()` are available).

## Out of Scope
- Implementation of the actual V-Sync logic (remains as a placeholder).
- Modification of the core `ProfilerManager` or `ProfilerTimeline` classes.
