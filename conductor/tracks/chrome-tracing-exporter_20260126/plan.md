# Plan: Chrome Tracing Exporter Implementation

This plan covers the implementation of the `ProfileExporter` class to support Chrome Trace Event Format export for the April engine's profiler.

## Phase 1: Foundation & Registry Enhancement
Prepare the `ProfileManager` to support thread naming and define the basic exporter structure.

- [x] Task: Enhance `ProfileManager` with a thread name registry. [d1aea28]
    - [x] Add `std::map<uint32_t, std::string> m_threadNames` to `ProfileManager`.
    - [x] Add `auto registerThreadName(uint32_t tid, std::string const& name) -> void`.
    - [x] Pre-register `0xFFFFFFFF` as "GPU Queue" in `ProfileManager` initialization.
- [ ] Task: Define `ProfileExporter` skeleton.
    - [ ] Create `engine/core/library/profiler_exporter.hpp` with the `exportToFile` static interface.
    - [ ] Create `engine/core/source/profiler_exporter.cpp`.
- [ ] Task: Conductor - User Manual Verification 'Phase 1' (Protocol in workflow.md)

## Phase 2: JSON Formatting & Export Logic
Implement the core conversion logic from `ProfileEvent` to Chrome Trace JSON.

- [ ] Task: Write TDD tests for JSON formatting.
    - [ ] Create `engine/test/source/test-profile-exporter.cpp`.
    - [ ] Define tests for timestamp conversion (ns to us).
    - [ ] Define tests for event type mapping (Begin/End).
    - [ ] Define tests for metadata event generation (thread names).
- [ ] Task: Implement `ProfileExporter::exportToFile`.
    - [ ] Use `std::ofstream` for efficient file writing.
    - [ ] Implement metadata event writing at the start of the JSON array.
    - [ ] Loop through `ProfileEvent` vector and format each as a JSON object.
    - [ ] Ensure correct JSON comma separation and array closure.
- [ ] Task: Verify TDD tests pass.
- [ ] Task: Conductor - User Manual Verification 'Phase 2' (Protocol in workflow.md)

## Phase 3: Integration & End-to-End Test
Verify the exporter works with real data collected from the `ProfileManager`.

- [ ] Task: Add an integration test in `test-profiler.cpp` or a new test.
    - [ ] Start profiling, record some dummy events, flush, and export.
    - [ ] Manually verify the output file `test_profile.json` in a Chrome Trace viewer.
- [ ] Task: Conductor - User Manual Verification 'Phase 3' (Protocol in workflow.md)
