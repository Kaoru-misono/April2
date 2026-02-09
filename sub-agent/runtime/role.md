# Runtime Role

## Summary
Engine loop orchestration: frame timing, window/device init, and scene render flow.

## In Scope
- Main loop and frame scheduling
- Window/device/swapchain initialization sequencing
- Driving scene update and render submission
- Runtime configuration and shutdown

## Out of Scope
- Low-level RHI implementation
- Editor UI widgets and tools

## Dependencies
- Depends on: core, graphics, scene, asset, ui
- Integration: agent/integration/threading-model.md

## Ownership Rules
- public interface changes require updating `interfaces.md` + `changelog.md`
- breaking changes require an ADR if cross-module impact exists
