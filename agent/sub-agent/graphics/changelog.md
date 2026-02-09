# Graphics Changelog

## Unreleased
- Added material ABI versioning in `MaterialHeader` (`abiVersion` + reserved fields) and expanded layout checks in generated C++ headers to guard size/offset compatibility.
- Extended `MaterialSystem` with descriptor-table registration/access APIs for textures, samplers, and buffers; `StandardMaterialData` now carries descriptor handles with safe fallback to invalid handle `0`.
