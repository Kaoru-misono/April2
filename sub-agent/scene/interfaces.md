# Scene Public Interfaces

## Purpose
ECS-driven scene data structures and render extraction utilities.

## Usage Patterns
- Use `Registry` to create entities and components, and `view()`/`each()` to iterate.
- Call `extractFrameSnapshot()` to build render data from a registry.
- Include `scene/scene.hpp` for a convenient bundle of scene headers.

## Public Header Index
- `scene/ecs-core.hpp` — Entity/component registry and sparse-set storage for ECS.
- `scene/renderer/render-extraction.hpp` — Render extraction entry point from ECS registry to frame snapshot.
- `scene/scene.hpp` — Convenience umbrella header for core scene types and renderer interfaces.

## Header Details

### scene/ecs-core.hpp
Location: `engine/scene/source/scene/ecs-core.hpp`
Include: `#include <scene/ecs-core.hpp>`

Purpose: Entity/component registry and sparse-set storage for ECS.

Key Types: `SparseSet`, `Entity`, `EntityHash`, `ISparseSet`, `View`, `EachRange`, `Iterator`, `MultiView`, `Registry`
Key APIs: `Registry::create()/destroy()`, `Registry::emplace()/get()`, `Registry::view()`, `Entity`

Usage Notes:
- Use `Registry` as the central ECS owner; entities are identified by index+generation.
- Use `view()` or `each()` to iterate over components efficiently.

Used By: `editor`

### scene/renderer/render-extraction.hpp
Location: `engine/scene/source/scene/renderer/render-extraction.hpp`
Include: `#include <scene/renderer/render-extraction.hpp>`

Purpose: Render extraction entry point from ECS registry to frame snapshot.

Key APIs: `extractFrameSnapshot(...)`

Usage Notes:
- Call once per frame to populate `FrameSnapshot` for rendering.

Used By: `runtime`

### scene/scene.hpp
Location: `engine/scene/source/scene/scene.hpp`
Include: `#include <scene/scene.hpp>`

Purpose: Convenience umbrella header for core scene types and renderer interfaces.

Usage Notes:
- Include this when you need the full scene surface in one header.

Used By: `editor`, `runtime`
