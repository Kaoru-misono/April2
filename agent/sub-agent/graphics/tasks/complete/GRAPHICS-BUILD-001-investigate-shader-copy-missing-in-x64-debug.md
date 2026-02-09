---
id: GRAPHICS-BUILD-001
title: Investigate missing shader directory copy in x64-debug bin output
status: done
owner: opencode
priority: p1
deps: []
updated_at: 2026-02-09
evidence: "Inspected `cmake/april-resources.cmake` and generated `build/x64-debug/build.ninja`. Shader sync is registered as `add_custom_command(TARGET <module> POST_BUILD ...)` on static library targets (for graphics at build.ninja line ~919). This only runs when target `April_graphics` is rebuilt/linked, so if `bin/shader` is deleted or stale while target is up-to-date, a normal incremental build may not restore shaders. Current local verification also shows build interruption before relink (`cmake --build build/x64-debug --target April_graphics` fails in `April_core` with missing C++ standard headers), which prevents POST_BUILD copy from running at all."
---

## Goal
Find why shader assets are not present under `build/x64-debug/bin/shader` after normal build flow.

## Acceptance Criteria
- [ ] Identify concrete root cause in build/runtime path.
- [ ] Provide reproducible verification evidence.
- [ ] Propose minimally invasive fix direction.

## Test Plan
- inspect: `cmake` scripts for shader sync command registration.
- inspect: generated `build/x64-debug/build.ninja` post-build commands.
- verify: check actual output under `build/x64-debug/bin/shader`.

## Expected Files
- `cmake/april-resources.cmake`
- `cmake/april-build.cmake`
- `build/x64-debug/build.ninja`
