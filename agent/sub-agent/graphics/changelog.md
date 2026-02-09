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
- Added `MaterialUpdateFlags` and per-material dirty tracking (`isDirty()`, `markDirty()`, `clearDirty()`) for selective GPU updates.
- Added `IMaterial::getTypeName()` for human-readable type names.
- Added `IMaterial::serializeParameters()` / `deserializeParameters()` for JSON round-trip serialization.
- Both `StandardMaterial` and `UnlitMaterial` now support parameter serialization for editor tooling.
- Added Falcor-style host `BasicMaterial` base abstraction and refactored `StandardMaterial` to inherit shared payload/texture/descriptor logic without introducing `MaterialType::Basic`.
- Standard BSDF parity update: dielectric F0 is now derived from IoR and roughness/metallic inputs are clamped in shader BSDF setup.
- Scene material pipeline now imports and uses the explicit `IMaterialInstance` contract in `scene-mesh.slang` while binding through `MaterialSystem` as the authoritative path.
- Program conformance/linking hardened: preflight now fails missing material conformances, composite-link diagnostics now include Slang details, and specialization-key generation is null-safe.
