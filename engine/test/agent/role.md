# Test Role

## Summary
Doctest-based tests and standalone test applications.

## In Scope
- Test harness and registration
- Unit/integration tests under engine/test
- GPU test apps for validation

## Out of Scope
- Production runtime features

## Dependencies
- Depends on: modules under test (core/graphics/runtime/etc.)

## Ownership Rules
- public interface changes require updating `interfaces.md` + `changelog.md`
- breaking changes require an ADR if cross-module impact exists
