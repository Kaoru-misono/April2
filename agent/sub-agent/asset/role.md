# Asset Role

## Summary
Asset system: importers, registry, and derived data cache (DDC).

## In Scope
- Asset registry and handles
- Importers for textures/materials/gltf
- Derived data cache and reimport rules
- Async asset load pipeline

## Out of Scope
- Rendering and GPU resource ownership
- Editor UI and tooling

## Dependencies
- Depends on: core, graphics
- Integration: agent/integration/asset-format.md
- Integration: agent/integration/threading-model.md

## Ownership Rules
- public interface changes require updating `interfaces.md` + `changelog.md`
- breaking changes require an ADR if cross-module impact exists
