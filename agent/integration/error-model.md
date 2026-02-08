# Error Model (cross-module contract)

## Goal
Unify error handling: avoid mixing throw/bool/log-only inconsistently.

## Recommended strategy
- Public API: prefer `Result<T>` (or equivalent) / error codes + message
- Internal unrecoverable bugs: assert/terminate with rich logging context

## Conventions
- Domain-based error codes: core / platform / render / asset / editor
- Errors must be diagnosable: include module/function + key parameter summary

## Compatibility
- Adding new error codes is non-breaking
- Changing error semantics is breaking and must be recorded in changelog
