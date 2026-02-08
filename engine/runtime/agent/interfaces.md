# Runtime Public Interfaces

## Purpose
Engine loop orchestration and access to core subsystems (window, device, asset, scene).

## Usage Patterns
- Configure via `EngineConfig`, provide hooks via `EngineHooks`, then call `Engine::run()`.
- Query subsystems via `Engine::get()` accessors (device, swapchain, scene, assets).

## Public Header Index
- `runtime/engine.hpp` — Engine loop entry point with config and hook callbacks.

## Header Details

### runtime/engine.hpp
Location: `engine/runtime/source/runtime/engine.hpp`
Include: `#include <runtime/engine.hpp>`

Purpose: Engine loop entry point with config and hook callbacks.

Key Types: `EngineConfig`, `EngineHooks`, `Engine`
Key APIs: `EngineConfig`, `EngineHooks`, `Engine::run()/stop()`, `Engine::get()`

Usage Notes:
- Create `Engine` with config and hooks, then call `run()`.
- Use accessors like `getDevice()` and `getSceneGraph()` to reach subsystems.

Used By: `editor`
