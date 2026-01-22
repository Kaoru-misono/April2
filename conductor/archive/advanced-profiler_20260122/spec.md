# Specification: Advanced Profiler Features & Visualization

## Overview
Expand the core profiling system to provide professional-grade visualization and analysis tools, achieving feature parity with reference implementations in `nvpro_core2` and `Falcor`. This includes hierarchical event tracking, integrated ImGui UI components, and rich data visualization using ImPlot.

## Functional Requirements
- **Hierarchical Profiling:** Support nested timing scopes with parent/child relationships.
- **ImGui Table View:** A searchable, sortable, and hierarchical table displaying detailed timing stats (Min, Max, Avg, Last).
- **ImPlot Visualization:**
    - Line charts for frame time history.
    - Stacked bar charts for identifying bottlenecks.
    - Pie charts for percentage of frame utilization.
- **Enhanced GPU Profiling:**
    - Support for multiple command lists and out-of-frame (async) GPU tasks.
    - Automatic resolver management.
    - Deep Slang RHI integration.
- **Persistent Settings:** Save/load UI layout and visible timers to `imgui.ini` or a custom config.

## Non-Functional Requirements
- **Low Overhead:** UI rendering should not significantly impact the timings it's displaying.
- **Modular UI:** The profiler widget should be an optional component that can be easily added to any engine application.

## Acceptance Criteria
- A hierarchical view is available in the UI.
- Historical data is graphed using ImPlot.
- GPU profiling supports nested markers and multiple queues.
- UI settings persist across sessions.
