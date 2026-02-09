# Core Role

## Summary
Foundational utilities: logging, profiling, math, file/IO, and platform abstractions.

## In Scope
- Logging, diagnostics, and profiling utilities
- Math/types shared across modules
- File system/VFS and path utilities
- Platform and window abstractions
- Common time and configuration helpers

## Out of Scope
- Rendering and GPU resource management
- Asset import and serialization
- Editor-specific tools or UI

## Dependencies
- Depends on: (none; base module)
- Integration: agent/integration/threading-model.md
- Integration: agent/integration/error-model.md

## Ownership Rules
- public interface changes require updating `interfaces.md` + `changelog.md`
- breaking changes require an ADR if cross-module impact exists
