# Specification - Profiler Phase 2: High-Performance Data Collection

## 1. Overview
This track implements a high-performance, lock-free data collection system for the April Engine profiler. By utilizing Thread Local Storage (TLS) and bump-pointer allocation, it ensures that instrumentation overhead is negligible even in high-frequency code blocks.

## 2. Functional Requirements

### 2.1 ProfileEvent Structure
- **Design:** Optimized for cache performance and minimal memory footprint.
- **Data Members:**
    - `uint64_t timestamp`: High-precision time from the `Timer` class.
    - `const char* name`: Pointer to the zone name (string literal).
    - `uint32_t threadId`: The ID of the thread that generated the event.
    - `uint8_t type`: Enum indicating a `Begin` or `End` event.
- **Optimization:** Use `alignas(32)` to ensure events are neatly packed into CPU cache lines, minimizing cache misses during processing.

### 2.2 Thread-Local Buffer (TLS)
- **Mechanism:** Each thread maintains its own `ProfileBuffer` (a pre-allocated array of `ProfileEvent`).
- **Performance:** Recording an event is a lock-free O(1) operation involving a single timestamp query and a pointer increment.
- **Auto-Registration:** When a thread first uses a profiling macro, its local buffer is automatically registered with the `ProfileManager`.

### 2.3 ProfileManager (Singleton)
- **Role:** Orchestrates data collection across all threads.
- **Thread Safety:** 
    - Mutex protected "cold path" for thread registration.
    - Lock-free "hot path" for thread-local recording.
- **Flush Logic:** 
    - Aggregates events from all active thread buffers into a frame-specific storage.
    - Resets thread-local write pointers for the next collection cycle.

## 3. Technical Strategy
- Implement `ProfileBuffer` as a fixed-size wrapper around `std::array` or raw memory to avoid allocations.
- Use `std::atomic` for thread-safe buffer pointer management during flushes.
- Ensure the `ProfileManager` gracefully handles thread termination by maintaining ownership of the buffer memory until the final flush.

## 4. Acceptance Criteria
- [ ] `ProfileEvent` structure matches the 32-byte alignment and size requirements.
- [ ] Multiple threads can profile simultaneously with zero lock contention.
- [ ] `ProfileManager::flush()` successfully gathers and resets data from all registered threads.
- [ ] Unit tests verify correct event ordering and integrity after a flush.

## 5. Out of Scope
- Hierarchical tree reconstruction (Phase 3).
- Real-time ImPlot visualization.
- Chrome Tracing JSON export.
