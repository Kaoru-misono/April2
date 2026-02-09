---
name: april-docsync-guard
description: Use when a change may affect public APIs, contracts, or module behavior in April2. Ensures documentation and changelog updates stay in sync with code.
---

# April DocSync Guard

## Trigger Conditions
- public headers or exported interfaces change
- protocol/format/integration contract changes
- behavior changes visible to other modules

## Procedure
1. Identify impacted module and surface area.
2. Update docs in `agent/sub-agent/<module>/interfaces.md`.
3. Update `agent/sub-agent/<module>/changelog.md` for public behavior.
4. For cross-module contracts, update files in `agent/integration/`.
5. Run `python agent/script/check-docsync.py` when possible.

## Rules
- Docs update must be in the same commit/PR as code change.
- If behavior changed but API did not, changelog still needs an entry.
- If contract is provisional, document expected request/response and error model.

## Outputs
- synchronized contract docs
- passing DocSync check (or explicit reason if not runnable)
