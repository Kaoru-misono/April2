# Graphics Role

## Summary
Rendering backend and RHI abstractions: device, swapchain, resources, and shaders.

## In Scope
- RHI device and swapchain management
- Command submission and GPU synchronization
- Shader compilation and pipeline setup
- GPU resource creation and lifetime management

## Out of Scope
- Scene graph and gameplay data
- Asset import pipelines
- Editor UI logic

## Dependencies
- Depends on: core
- Integration: agent/integration/threading-model.md
- Integration: agent/integration/error-model.md

## Ownership Rules
- public interface changes require updating `interfaces.md` + `changelog.md`
- breaking changes require an ADR if cross-module impact exists
