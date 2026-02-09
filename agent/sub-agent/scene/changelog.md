# Scene Changelog

## Unreleased
- `RenderResourceRegistry::registerMaterialAsset()` now uses `MaterialAsset::materialType` for material class selection instead of path-based heuristics; warns on unknown types.
