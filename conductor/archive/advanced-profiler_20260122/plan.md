# Implementation Plan - Advanced Profiler Features

## Phase 1: Hierarchical Data & Infrastructure [checkpoint: d81aba4]
Establish the data structures needed for complex visualization.

- [x] Task: Implement Hierarchical Event Tracking. [a348aba]
    - [x] Update `Profiler` to use a per-thread stack for tracking nested scopes.
    - [x] Update `Snapshot` structure to represent a tree instead of a flat list.
- [x] Task: Implement Advanced GPU Sync & Async Sections. [ebba5e8]
    - [x] Support nested GPU markers.
    - [x] Implement `AsyncSection` for tasks outside the main frame loop.

## Phase 2: ImGui & ImPlot Visualization [checkpoint: 60344]
Build the user-facing analysis tools.

- [x] Task: Implement Hierarchical ImGui Table.
    - [x] Create a tree-table view showing nested timers.
    - [x] Display Min/Max/Avg/Last columns.
- [x] Task: Implement Rich Visualization with ImPlot.
    - [x] Line chart for total CPU/GPU frame time.
    - [x] Individual timer tracking in a line chart.
    - [x] Stacked bar charts for frame composition.

## Phase 3: Polish & Integration
Finalize the developer experience.

- [x] Task: Implement UI Persistence & Customization.
    - [x] Save/load visible tracks and UI state.
    - [x] Add search/filter functionality to the profiler table.
- [x] Task: Integration & Examples.
    - [x] Integrate the new profiler widget into the main engine editor.
    - [x] Create a comprehensive test/example showing complex nested CPU/GPU workloads.

