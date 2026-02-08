# Agent Workflow (April2)

This repository uses Markdown to drive multi-agent parallel development. The goals:
- stable collaboration boundaries
- low coupling
- prevent knowledge loss
- reduce rework

## Document Layers (important)
- **integration/**: cross-module contracts (protocols/formats/threading/error model). Single source of truth.
- **<module>/agent/interfaces.md**: module public interface docs aggregated per module (one file listing all public headers).
  - Public headers are those included via `#include <module/...>` from other engine modules (excluding `entry/` and `engine/test`).
- **<module>/agent/knowledge.md**: pitfalls, debugging, performance, platform notes.
- **<module>/agent/tasks.md**: actionable task cards to drive agents.
- **decisions/**: ADRs (why we decided X).

## Change Rules
1. Any PR that changes **public behavior / API / protocol / format** must update the corresponding docs:
   - module public API: update `<module>/agent/interfaces.md` and `<module>/agent/changelog.md`
   - cross-module protocol/format: update `/agent/integration/*` and reference it from affected modules
2. **DocSync (anti-drift)**: CI blocks PRs that change public API without updating the required docs.
3. Prefer small PRs: **stub-first** (contract + stub + buildable runnable demo/test), then incrementally implement.

## Multi-Agent Work Rules
1. **Read first**: before coding, every agent must read its module's `role.md` and `knowledge.md`.
2. **Understand the module**: scan the module's current structure (source layout, key headers, and build files) to ground the work.
3. **Plan-driven work**:
   - Start from the module's `plan/` and break the plan into concrete items in `tasks.md`.
   - Each task should be independently mergeable and mapped to clear output files.
4. **Cross-module changes**:
   - If your task requires changes in another module, add a **specific request** to that module's `plan/` (do not block your own work).
   - The request must define the interface contract (names, parameters, return types, error behavior, threading expectations).
   - Assume the other module will implement the request correctly; continue your own work against the contract.
5. **Execution and handoff**:
   - Complete your tasks and then wait.
   - A test agent will run tests. If failures occur, create a new plan item for the responsible module and let that module's agent fix and refine.

## Mandatory Housekeeping (MUST)
Before coding:
1) Pick exactly ONE task file in `agent/tasks/`.
2) Set `status: in_progress`, update `updated_at`.
3) If the task depends on missing contracts/APIs, set `status: blocked` and write what is missing.

After coding:
1) Update the same task file:
   - if completed: `status: done`
   - fill `evidence` with PR link / commit hash + how to run the demo/test
2) If follow-up work is discovered, create NEW task file(s) (do not bloat the current one).


## Definition of Done (DoD)
- ✅ build passes
- ✅ tests or at least one demo path passes
- ✅ docs updated (interfaces/protocols/changelog)
- ✅ no hidden cross-module dependencies (only contract-level interaction)

## Suggested commit granularity
- commit 1: contract + stub + demo/test
- commit 2: implementation A
- commit 3: implementation B
- final: rebase + cleanups + docs polish
