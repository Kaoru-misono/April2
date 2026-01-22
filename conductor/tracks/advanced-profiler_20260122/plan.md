# Implementation Plan - Advanced Profiler Features

## Phase 1: Hierarchical Data & Infrastructure
Establish the data structures needed for complex visualization.

- [ ] Task: Implement Hierarchical Event Tracking.
    - [ ] Update `Profiler` to use a per-thread stack for tracking nested scopes.
    - [ ] Update `Snapshot` structure to represent a tree instead of a flat list.
- [ ] Task: Implement Advanced GPU Sync & Async Sections.
    - [ ] Support nested GPU markers.
    - [ ] Implement `AsyncSection` for tasks outside the main frame loop.

## Phase 2: ImGui & ImPlot Visualization
Build the user-facing analysis tools.

- [ ] Task: Implement Hierarchical ImGui Table.
    - [ ] Create a tree-table view showing nested timers.
    - [ ] Display Min/Max/Avg/Last columns.
- [ ] Task: Implement Rich Visualization with ImPlot.
    - [ ] Line chart for total CPU/GPU frame time.
    - [ ] Individual timer tracking in a line chart.
    - [ ] Stacked bar charts for frame composition.

## Phase 3: Polish & Integration
Finalize the developer experience.

- [ ] Task: Implement UI Persistence & Customization.
    - [ ] Save/load visible tracks and UI state.
    - [ ] Add search/filter functionality to the profiler table.
- [ ] Task: Integration & Examples.
    - [ ] Integrate the new profiler widget into the main engine editor.
    - [ ] Create a comprehensive test/example showing complex nested CPU/GPU workloads.
