# Graphics Knowledge

## Pitfalls
- Submitting commands from the wrong thread
- Resource lifetime crossing frame boundaries

## Performance
- Avoid per-frame pipeline creation
- Batch resource updates

## Debugging
- Enable validation layers when available
- Capture a frame before large refactors

## Platform Notes
- Backend differences may require platform-specific guards

## Build Regression Notes
- After updating Slang enums/structs annotated with `@export-cpp`, always regenerate local generated headers before compiling (for example `material-types.generated.hpp`), otherwise host C++ may reference missing generated enum entries.
- `MaterialAsset::getAssetPath()` is a `std::string` in current host path. Do not call `.string()` on it; lowercasing should operate directly on the string buffer.
- For material extension changes, validate both shader and host paths in one pass: enum id in Slang, generated header refresh, and host material type usage (`generated::MaterialType::*`).

## Falcor Material Parity Lessons
- Program creation must inject both material shader modules and material shader defines. Adding only type conformances causes link failures like `Type 'StandardMaterial' in type conformance was not found`.
- Follow Falcor conformance granularity: material-level conformance (`StandardMaterial -> IMaterial`, `UnlitMaterial -> IMaterial`) is required. Do not add internal BRDF implementation conformances from host unless shader interface actually dispatches on them.
- Keep shader-module paths consistent with runtime VFS (`graphics/...`). Mixing `engine/graphics/shader/...` and `graphics/...` leads to module resolution drift.
- `IMaterial` and `IMaterialInstance` size limits are independent and must match real payload sizes:
  - `IMaterial` should stay at `[anyValueSize(128)]` for Falcor parity.
  - Material implementation payload should therefore be compact (use `materialID`, not full `StandardMaterialData` blob).
  - `IMaterialInstance` must use an explicit `FALCOR_MATERIAL_INSTANCE_SIZE` define that is also passed from host program defines.
- For DXC backends, large resource arrays in MaterialSystem context (`ByteAddressBuffer[]`, `Texture3D[]`) can trigger incomplete-type failures after specialization/flattening. Keep extras gated by defines and default them off unless fully wired.
- Validate shader changes with both Slang front-end typecheck and DXIL compilation path. `slangc -no-codegen` alone can miss backend-specific codegen failures.
