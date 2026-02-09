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
