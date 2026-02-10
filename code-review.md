# Material Refactor Review Guide

This review order is optimized for understanding architecture first, then behavior, then stability.

## 1) `e1f2acd`
- **Focus:** Host-side `BasicMaterial` base abstraction (Falcor-aligned foundation)
- **Key files:**
  - `engine/graphics/source/graphics/material/basic-material.hpp`
  - `engine/graphics/source/graphics/material/basic-material.cpp`
  - `engine/graphics/source/graphics/material/standard-material.hpp`
  - `engine/graphics/source/graphics/material/standard-material.cpp`
  - `engine/graphics/source/graphics/material/material-system.cpp`
- **Review points:**
  - Common logic is actually moved into the base class (flags, texture binding, descriptor handles, shared serialization).
  - `StandardMaterial` keeps only standard-material-specific logic and data.

## 2) `b7ec0c1`
- **Focus:** Architecture correction confirmation at task level
- **Key file:**
  - `agent/sub-agent/graphics/tasks/complete/GRAPHICS-MATERIAL-609-falcor-basic-material-base-alignment.md`
- **Review points:**
  - Confirms `BasicMaterial` is a host abstraction, not a runtime `MaterialType` enum entry.

## 3) `59fda6c`
- **Focus:** Standard BSDF physical parity with Falcor
- **Key file:**
  - `engine/graphics/shader/material/standard-bsdf.slang`
- **Review points:**
  - Dielectric F0 derived from IoR (instead of hardcoded 0.04).
  - Roughness/metallic clamping is correct and non-breaking.
  - Host-to-shader parameter mapping remains consistent.

## 4) `06c8ca5`
- **Focus:** Render pipeline contract clarity
- **Key file:**
  - `engine/graphics/shader/scene/scene-mesh.slang`
- **Review points:**
  - Scene path explicitly consumes the `IMaterialInstance` contract.
  - No hidden or implicit coupling to legacy material assumptions.

## 5) `e2f1e13`
- **Focus:** Program/RHI conformance handling and diagnostics hardening
- **Key files:**
  - `engine/graphics/source/graphics/program/program.cpp`
  - `engine/graphics/source/graphics/program/program-manager.cpp`
  - `engine/graphics/source/graphics/program/program-version.cpp`
  - `engine/graphics/shader/material/material-factory.slang`
- **Review points:**
  - Conformance preflight is now an actionable gate for invalid material interface setup.
  - Slang composite/specialization/link failures provide useful diagnostics.
  - Specialization key generation is null-safe and avoids unstable behavior.

## 6) `a4d5d3a`
- **Focus:** Documentation and task closeout
- **Key files:**
  - `agent/sub-agent/graphics/interfaces.md`
  - `agent/sub-agent/graphics/changelog.md`
  - `agent/task-commit-tracker.md`
- **Review points:**
  - Code and docs are synchronized.
  - Task-to-commit mapping is complete for the batch.

## Recommended Fast Path
If you want a fast but high-value review, start with:
1. `e1f2acd` (architecture foundation)
2. `e2f1e13` (stability and diagnostics)

These two commits cover most of the architectural and runtime risk.
