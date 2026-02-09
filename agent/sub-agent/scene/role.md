# Scene Role

## Summary
Scene graph and entity/component data model used by runtime and editor.

## In Scope
- Scene graph and hierarchy
- Entity/component lifecycle and queries
- Transform management and scene data extraction

## Out of Scope
- Low-level rendering and RHI
- Asset import pipelines
- Editor UI logic

## Dependencies
- Depends on: core, graphics, asset
- Integration: agent/integration/threading-model.md

## Ownership Rules
- public interface changes require updating `interfaces.md` + `changelog.md`
- breaking changes require an ADR if cross-module impact exists
