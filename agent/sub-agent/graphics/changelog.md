# Graphics Changelog

## Unreleased
- Added material ABI versioning in `MaterialHeader` (`abiVersion` + reserved fields) and expanded layout checks in generated C++ headers to guard size/offset compatibility.
- Extended `MaterialSystem` with descriptor-table registration/access APIs for textures, samplers, and buffers; `StandardMaterialData` now carries descriptor handles with safe fallback to invalid handle `0`.
- Added host material-type registry and extension path (`Standard`, `Unlit`) plus shader-side material factory/instance flow for interface-based forward shading.
- Inspector now exposes material debug surface values (GPU material index, material type name/id) through render resource mapping APIs.
- Added `Program::validateConformancesPreflight()` to detect missing `IMaterialInstance` conformances before shader link (warning-only).
