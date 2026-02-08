# Core Public Interfaces

## Purpose
Foundational utilities for logging, assertions, IO, math types, profiling, and window/input used across the engine.

## Usage Patterns
- Use `core::ref<T>` and `APRIL_OBJECT` for ref-counted engine objects.
- Use `AP_*` logging macros and `AP_ASSERT` for diagnostics.
- Initialize and mount the VFS before reading content.
- Input is frame-based: call `Input::beginFrame()` and feed events each frame.
- Use `APRIL_PROFILE_ZONE` and `Profiler` to emit profiling events.

## Public Header Index
- `core/error/assert.hpp` — Assertion and unreachable macros integrated with logging and debug break.
- `core/file/vfs.hpp` — Virtual file system with alias mounts and file IO helpers.
- `core/foundation/object.hpp` — Reference-counted `Object` base and `core::ref<T>` smart pointer.
- `core/input/input.hpp` — Static input state accumulator for keyboard and mouse.
- `core/log/log-sink.hpp` — ILogSink interface for log output targets.
- `core/log/logger.hpp` — Logger implementation and global logging macros.
- `core/math/json.hpp` — nlohmann::json serializers for glm vectors, matrices, and quaternions.
- `core/math/math.hpp` — Math helpers on top of GLM (lerp, saturate, transforms).
- `core/math/type.hpp` — GLM type aliases (float2/float3/float4, matrices, quaternion).
- `core/profile/profile-aggregator.hpp` — Aggregates profile events into per-thread call trees.
- `core/profile/profile-manager.hpp` — Singleton coordinator for profile buffers and thread naming.
- `core/profile/profiler.hpp` — Profiler core API and scoped profiling zones.
- `core/profile/timer.hpp` — High-precision timing utilities.
- `core/tools/alignment.hpp` — Alignment helpers for power-of-two boundaries.
- `core/tools/enum-flags.hpp` — Bitmask enum operators and flag utilities.
- `core/tools/enum.hpp` — Enum reflection helpers and string conversions (plus std::format support).
- `core/tools/hash.hpp` — Hash utilities for hashable types and strings.
- `core/tools/sha1.hpp` — SHA-1 hashing utility.
- `core/tools/uuid.hpp` — UUID wrapper with string conversion and std::hash specialization.
- `core/window/window.hpp` — Window abstraction and event subscription interface.

## Header Details

### core/error/assert.hpp
Location: `engine/core/source/core/error/assert.hpp`
Include: `#include <core/error/assert.hpp>`

Purpose: Assertion and unreachable macros integrated with logging and debug break.

Key APIs: `AP_ASSERT(...)`, `AP_UNREACHABLE(...)`, `AP_DEBUGBREAK()`, `AP_ENABLE_ASSERTS`

Usage Notes:
- `AP_ASSERT` is compiled out when `AP_ENABLE_ASSERTS` is 0.
- `AP_UNREACHABLE` is always active and logs before breaking.

Used By: `asset`, `graphics`, `runtime`, `scene`

### core/file/vfs.hpp
Location: `engine/core/source/core/file/vfs.hpp`
Include: `#include <core/file/vfs.hpp>`

Purpose: Virtual file system with alias mounts and file IO helpers.

Key Types: `File`, `VFS`
Key APIs: `VFS::init()/shutdown()`, `VFS::mount()/unmount()`, `VFS::open()`, `VFS::readTextFile()/readBinaryFile()`, `VFS::writeTextFile()/writeBinaryFile()`, `VFS::listFilesRecursive()`

Usage Notes:
- Initialize and mount aliases before reading content paths.
- Use `open()` for streaming access; convenience methods read entire files.

Used By: `asset`, `editor`, `graphics`

### core/foundation/object.hpp
Location: `engine/core/source/core/foundation/object.hpp`
Include: `#include <core/foundation/object.hpp>`

Purpose: Reference-counted `Object` base and `core::ref<T>` smart pointer.

Key Types: `Object`, `RefTracker`, `ref`, `BreakableReference`
Key APIs: `core::Object`, `core::ref<T>`, `APRIL_OBJECT`

Usage Notes:
- Derive from `core::Object` and declare `APRIL_OBJECT` in derived classes.
- Use `core::ref<T>` for shared ownership; avoid manual delete.

Used By: `editor`, `graphics`, `runtime`, `scene`

### core/input/input.hpp
Location: `engine/core/source/core/input/input.hpp`
Include: `#include <core/input/input.hpp>`

Purpose: Static input state accumulator for keyboard and mouse.

Key Types: `Input`
Key APIs: `Input::beginFrame()`, `Input::setKeyDown()/setMouseButtonDown()`, `Input::setMousePosition()/addMouseWheel()`, `Input::isKeyDown()/wasKeyPressed()`

Usage Notes:
- Call `beginFrame()` once per frame before applying new input events.
- Feed events from the window layer, then query in update code.

Used By: `editor`, `runtime`

### core/log/log-sink.hpp
Location: `engine/core/source/core/log/log-sink.hpp`
Include: `#include <core/log/log-sink.hpp>`

Purpose: ILogSink interface for log output targets.

Key Types: `ILogSink`
Key APIs: `ILogSink::log(...)`

Usage Notes:
- Implement `ILogSink` and register instances with `Logger::addSink()`.

Used By: `editor`

### core/log/logger.hpp
Location: `engine/core/source/core/log/logger.hpp`
Include: `#include <core/log/logger.hpp>`

Purpose: Logger implementation and global logging macros.

Key Types: `Logger`, `Log`
Key APIs: `Logger::addSink()/removeSink()`, `Logger::setLevel()`, `Log::getLogger()`, `AP_TRACE/AP_DEBUG/AP_INFO/AP_WARN/AP_ERROR/AP_FATAL/AP_CRITICAL`

Usage Notes:
- Prefer `AP_*` macros for consistent logging with source locations.
- Configure sinks and minimum level via `Logger`/`Log::getLogger()`.

Used By: `asset`, `editor`, `graphics`, `runtime`, `scene`

### core/math/json.hpp
Location: `engine/core/source/core/math/json.hpp`
Include: `#include <core/math/json.hpp>`

Purpose: nlohmann::json serializers for glm vectors, matrices, and quaternions.

Key Types: `adl_serializer`
Key APIs: `nlohmann::adl_serializer<glm::vec/mat/qua>`

Usage Notes:
- Include this header before serializing glm types with nlohmann::json.

Used By: `asset`

### core/math/math.hpp
Location: `engine/core/source/core/math/math.hpp`
Include: `#include <core/math/math.hpp>`

Purpose: Math helpers on top of GLM (lerp, saturate, transforms).

Key APIs: `lerp()`, `saturate()`, `rotationFromAngleAxis()`, `translationFromPosition()`, `inverseTranspose()`, `divideRoundingUp()`

Usage Notes:
- Includes `core/math/type.hpp` for glm aliases.

Used By: `graphics`

### core/math/type.hpp
Location: `engine/core/source/core/math/type.hpp`
Include: `#include <core/math/type.hpp>`

Purpose: GLM type aliases (float2/float3/float4, matrices, quaternion).

Key APIs: `float2/float3/float4`, `float4x4`, `quaternion`

Usage Notes:
- Use these aliases to keep math types consistent across modules.

Used By: `asset`, `editor`, `graphics`, `runtime`, `scene`

### core/profile/profile-aggregator.hpp
Location: `engine/core/source/core/profile/profile-aggregator.hpp`
Include: `#include <core/profile/profile-aggregator.hpp>`

Purpose: Aggregates profile events into per-thread call trees.

Key Types: `ProfileNode`, `ProfileThreadFrame`, `ProfileAggregator`, `StatHistory`, `StackEntry`
Key APIs: `ProfileAggregator::ingest(...)`, `ProfileAggregator::getFrames()`, `ProfileAggregator::clear()`

Usage Notes:
- Feed events from `ProfileManager::flush()` and render `ProfileThreadFrame` trees.

Used By: `editor`

### core/profile/profile-manager.hpp
Location: `engine/core/source/core/profile/profile-manager.hpp`
Include: `#include <core/profile/profile-manager.hpp>`

Purpose: Singleton coordinator for profile buffers and thread naming.

Key Types: `ProfileManager`
Key APIs: `ProfileManager::registerBuffer()/unregisterBuffer()`, `ProfileManager::registerThreadName()`, `ProfileManager::flush()`

Usage Notes:
- Register thread-local buffers; call `flush()` to collect and clear events.

Used By: `editor`

### core/profile/profiler.hpp
Location: `engine/core/source/core/profile/profiler.hpp`
Include: `#include <core/profile/profiler.hpp>`

Purpose: Profiler core API and scoped profiling zones.

Key Types: `IGpuProfiler`, `Profiler`, `ScopedProfileZone`
Key APIs: `Profiler::get()`, `Profiler::recordEvent(...)`, `Profiler::registerGpuProfiler(...)`, `ScopedProfileZone`, `APRIL_PROFILE_ZONE`

Usage Notes:
- Use `APRIL_PROFILE_ZONE` or `ScopedProfileZone` to emit CPU profiling events.
- Register an `IGpuProfiler` provider for GPU timelines.

Used By: `graphics`

### core/profile/timer.hpp
Location: `engine/core/source/core/profile/timer.hpp`
Include: `#include <core/profile/timer.hpp>`

Purpose: High-precision timing utilities.

Key Types: `Timer`
Key APIs: `Timer::now()`, `Timer::calcDuration()`, `Timer::calcDurationMicroseconds()`, `Timer::calcDurationNanoseconds()`

Usage Notes:
- Use `Timer::now()` and duration helpers to measure intervals.

Used By: `graphics`

### core/tools/alignment.hpp
Location: `engine/core/source/core/tools/alignment.hpp`
Include: `#include <core/tools/alignment.hpp>`

Purpose: Alignment helpers for power-of-two boundaries.

Key APIs: `is_aligned()`, `align_up()`, `align_down()`

Usage Notes:
- `a` must be a power of two for all alignment helpers.

Used By: `graphics`

### core/tools/enum-flags.hpp
Location: `engine/core/source/core/tools/enum-flags.hpp`
Include: `#include <core/tools/enum-flags.hpp>`

Purpose: Bitmask enum operators and flag utilities.

Key APIs: `AP_ENUM_CLASS_OPERATORS`, `enum_has_all_flags()`, `enum_has_any_flags()`, `enum_add_flags()`, `enum_remove_flags()`

Usage Notes:
- Apply `AP_ENUM_CLASS_OPERATORS` to enable bitwise ops on enum classes.

Used By: `graphics`

### core/tools/enum.hpp
Location: `engine/core/source/core/tools/enum.hpp`
Include: `#include <core/tools/enum.hpp>`

Purpose: Enum reflection helpers and string conversions (plus std::format support).

Key APIs: `AP_ENUM_INFO`, `AP_ENUM_REGISTER`, `enumToString()`, `stringToEnum()`, `flagsToStringList()`, `stringListToFlags()`

Usage Notes:
- Declare `AP_ENUM_INFO` and `AP_ENUM_REGISTER` in the enum namespace.

Used By: `graphics`

### core/tools/hash.hpp
Location: `engine/core/source/core/tools/hash.hpp`
Include: `#include <core/tools/hash.hpp>`

Purpose: Hash utilities for hashable types and strings.

Key APIs: `core::hash(...)`, `core::hash_combine(...)`, `core::computeStringHash(...)`

Usage Notes:
- Use `core::hash()` for types that provide `hash()` or std::hash.

Used By: `asset`, `graphics`

### core/tools/sha1.hpp
Location: `engine/core/source/core/tools/sha1.hpp`
Include: `#include <core/tools/sha1.hpp>`

Purpose: SHA-1 hashing utility.

Key Types: `Sha1`
Key APIs: `Sha1::update(...)`, `Sha1::getDigest()`, `Sha1::getHexDigest()`, `Sha1::reset()`

Usage Notes:
- Call `update()` with data, then fetch `getHexDigest()` for hex output.

Used By: `asset`

### core/tools/uuid.hpp
Location: `engine/core/source/core/tools/uuid.hpp`
Include: `#include <core/tools/uuid.hpp>`

Purpose: UUID wrapper with string conversion and std::hash specialization.

Key Types: `UUID`
Key APIs: `core::UUID`, `UUID::toString()`

Usage Notes:
- Default constructor generates a random UUID; string ctor parses a UUID string.

Used By: `asset`

### core/window/window.hpp
Location: `engine/core/source/core/window/window.hpp`
Include: `#include <core/window/window.hpp>`

Purpose: Window abstraction and event subscription interface.

Key Types: `EWindowType`, `WindowDesc`, `Window`
Key APIs: `Window::create(...)`, `Window::subscribe<T>(...)`, `Window::setVSync()/isVSync()`, `WindowDesc`

Usage Notes:
- Call `Window::create()` to obtain a backend window instance.
- Use `subscribe<T>` to register typed event callbacks.

Used By: `editor`, `graphics`, `runtime`
