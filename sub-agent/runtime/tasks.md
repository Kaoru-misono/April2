# Runtime Tasks

> Every task must be independently mergeable. Avoid big-bang PRs.

## Task Template
- ID: RT-000
- Goal: Define the runtime loop and lifecycle contract.
- Dependencies: (none)
- Scope: Document runtime public headers in `sub-agent/runtime/interfaces.md`.
- Out of scope: No runtime behavior changes.
- Acceptance Criteria:
- `sub-agent/runtime/interfaces.md` lists all public headers
- `changelog.md` updated if APIs change
- Test Plan: N/A (docs only)
- Expected files touched:
- `sub-agent/runtime/interfaces.md`
- `sub-agent/runtime/changelog.md`
- Risk: Low
- Owner: TBD

---

## Backlog
- RT-001 Document frame timing configuration
- RT-002 Add a headless runtime smoke test
