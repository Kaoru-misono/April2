# Specification: Chrome Tracing Exporter

## Overview
Implement the fifth phase of the April engine profiler: a Chrome Tracing exporter. This track focuses on creating a `ProfileExporter` class that converts internal `ProfileEvent` data into the JSON format used by the Chrome Trace Viewer (`chrome://tracing` or [ui.perfetto.dev](https://ui.perfetto.dev)).

## Functional Requirements
- **`ProfileExporter` Class**: Create a static or utility class in the `Core` module (or within the profiler directory).
- **Interface**: Provide a method `static auto exportToFile(std::string const& path, std::vector<ProfileEvent> const& events) -> void`.
- **JSON Mapping**:
    - `ProfileEventType::Begin` -> `"ph": "B"`
    - `ProfileEventType::End` -> `"ph": "E"`
    - **Timestamp**: Convert nanoseconds (internal) to microseconds (Chrome Tracing requirement).
    - **Thread ID**: Map `threadId` to `"tid"`.
- **Thread Metadata**:
    - Include `process_name` and `thread_name` metadata events in the JSON output.
    - **GPU Events**: Specifically handle `threadId == 0xFFFFFFFF` by labeling it as "GPU Queue" in the metadata.
    - **Registry Lookup**: `ProfileExporter` will internally fetch thread names/mappings from a registry (likely residing in `ProfileManager`).
- **File Output**: Use `std::ofstream` to write valid JSON to the specified path.

## Non-Functional Requirements
- **Performance**: Use efficient string formatting (e.g., `std::format` or pre-allocated buffers) and buffered file I/O to minimize export time.
- **Code Style**: 
    - C++23 standards.
    - Trailing return types (`auto func() -> void`).
    - `m_` prefix for member variables.
    - East Const (`int const` instead of `const int`).

## Acceptance Criteria
- [ ] A `ProfileExporter` class exists with the specified `exportToFile` interface.
- [ ] Exported JSON files can be successfully loaded and visualized in `chrome://tracing` or `ui.perfetto.dev`.
- [ ] GPU events are correctly grouped under a "GPU Queue" thread name.
- [ ] Timestamps are correctly scaled from nanoseconds to microseconds.
- [ ] Unit tests verify the JSON structure for a sample set of events.

## Out of Scope
- Integration with the ImGui UI (buttons, file dialogs).
- Live streaming of events (this is a post-flush "offline" export).
- Exporting arguments/properties for events (basic timing only).
