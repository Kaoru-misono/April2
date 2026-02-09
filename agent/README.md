# Skill + Sub-Agent Workflow (April2)

This repository now treats `agent/` as the source of truth for a Skill-first workflow with module sub-agents.

## Goals
- stable collaboration boundaries
- low coupling across modules
- reusable execution playbooks (skills)
- less context loss between coding sessions

## Layout
- `agent/sub-agent/<module>/` module knowledge base and task cards.
  - `role.md`: ownership and boundaries.
  - `knowledge.md`: pitfalls, debugging, performance notes.
  - `interfaces.md`: public API contract index.
  - `changelog.md`: public behavior changes.
  - `plan/` + `tasks/` or `tasks.md`: mergeable work items.
- `agent/sub-agent/task-schema.md` canonical frontmatter schema for task cards.
- `agent/integration/` cross-module protocol and runtime contracts.
- `agent/decisions/` architecture decision records.
- `.agents/skills/` reusable skills (Codex-compatible Agent Skills).
- `.claude/agents/` project sub-agents (Claude Code-compatible agents).

## Core Rules
1. Any PR that changes public behavior/API/protocol/format must update docs in the same change.
2. DocSync (`agent/.docsync.yml`) is the anti-drift gate.
3. Prefer small, mergeable increments: contract + stub + validation first, then implementation.

## Skill-Driven Execution
1. Load the relevant workflow skills first (task lifecycle, docsync, cross-module contracts).
2. Route work to the matching module sub-agent.
3. The sub-agent reads `role.md` + `knowledge.md` before editing code.
4. The sub-agent updates exactly one active task card while executing.
5. On completion, the sub-agent updates evidence and hands off to test validation.

## Task Lifecycle (MUST)
Before coding:
1) Select exactly one task card for active work.
2) Set `status: in_progress` and refresh `updated_at`.
3) If required contracts are missing, set `status: blocked` and document the missing contract.

After coding:
1) Update the same task card:
   - completed work -> `status: done`
   - add `evidence` (commit hash/PR + verification command)
2) Create new task cards for follow-up work; do not overload the current card.

## Definition of Done
- build passes
- tests (or one demo path) pass
- docs updated (`interfaces`/`integration`/`changelog`)
- no hidden cross-module dependency beyond declared contracts

## Suggested Commit Granularity
- commit 1: contract + stub + test/demo path
- commit 2+: implementation slices
- final: cleanup + docs polish
