# Editor Role

## Summary
Editor application layer: tools, panels, state management, and RPC integration.

## In Scope
- Editor panels, tools, and commands
- Selection, undo/redo, and editor state
- Editor <-> engine RPC integration

## Out of Scope
- Low-level rendering and RHI
- Asset import implementation details

## Dependencies
- Depends on: core, graphics, runtime, ui, scene, asset
- Integration: agent/integration/editor-rpc-mode.md
- Integration: agent/integration/threading-model.md

## Ownership Rules
- public interface changes require updating `interfaces.md` + `changelog.md`
- breaking changes require an ADR if cross-module impact exists
