# Graphics Changelog

## Unreleased
- Added material ABI versioning in `MaterialHeader` (`abiVersion` + reserved fields) and expanded layout checks in generated C++ headers to guard size/offset compatibility.
- Extended `MaterialSystem` with descriptor-table registration/access APIs for textures, samplers, and buffers; `StandardMaterialData` now carries descriptor handles with safe fallback to invalid handle `0`.
- Added host material-type registry and extension path (`Standard`, `Unlit`) plus shader-side material factory/instance flow for interface-based forward shading.
- Inspector now exposes material debug surface values (GPU material index, material type name/id) through render resource mapping APIs.
- Added `Program::validateConformancesPreflight()` to detect missing `IMaterialInstance` conformances before shader link (warning-only).
- Descriptor table capacities are now host-configurable via `MaterialSystemConfig` and synchronized to shaders via `MATERIAL_TEXTURE_TABLE_SIZE`, `MATERIAL_SAMPLER_TABLE_SIZE`, `MATERIAL_BUFFER_TABLE_SIZE` defines.
- Added `MaterialSystem::getDiagnostics()` for material count by type, descriptor usage/capacity, and overflow counters.
- Inspector now shows Material System diagnostics section with descriptor usage and overflow warnings.
