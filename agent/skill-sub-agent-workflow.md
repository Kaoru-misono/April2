# Migration: Agent Docs -> Skill + Sub-Agent

This document maps the previous doc-driven agent workflow to a reusable Skill + Sub-Agent setup.

## What Changed
- Kept module knowledge in `agent/sub-agent/<module>/`.
- Added reusable workflow skills in `.agents/skills/`.
- Added project sub-agent definitions in `.claude/agents/`.
- Updated DocSync paths to point at `agent/sub-agent/` docs.

## Mapping
- `role.md` + `knowledge.md` -> consumed by module sub-agents at startup.
- `interfaces.md` + `changelog.md` -> guarded by `april-docsync-guard` skill.
- `tasks.md`/`tasks/*.md` -> managed by `april-task-lifecycle` skill.
- `integration/*.md` -> updated through `april-cross-module-contract` skill.

## Suggested Routing
- single-module task -> matching module agent (`april-core`, `april-graphics`, etc.)
- multi-module task -> `april-coordinator`, then delegate to module agents
- validation-focused task -> `april-test`

## Notes for Codex
- Skills are loaded from `.agents/skills/*/SKILL.md`.
- Skill descriptions are written for implicit matching and explicit invocation.

## Notes for Claude Code
- Project sub-agents are under `.claude/agents/*.md`.
- Each sub-agent references the same module knowledge base in `agent/sub-agent/`.
