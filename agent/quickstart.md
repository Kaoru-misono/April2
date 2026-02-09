# Skill + Sub-Agent Quickstart

Use this guide to run the new workflow in daily development.

## 1) Pick execution mode
- Single-module work: use the module agent directly (`april-core`, `april-graphics`, `april-editor`, etc.).
- Multi-module work: use `april-coordinator` and let it delegate.
- Test/regression work: use `april-test`.

## 2) Skill-first flow (Codex)
- Skills are auto-discovered from `.agents/skills/*/SKILL.md`.
- Explicitly invoke when needed:
  - `Use skill april-task-lifecycle to run this task`
  - `Use skill april-docsync-guard before finishing`
  - `Use skill april-cross-module-contract for scene->graphics dependency`

## 3) Sub-agent flow (Claude Code)
- Sub-agents live in `.claude/agents/`.
- Example prompts:
  - `Use april-graphics to implement GRAPHICS-MATERIAL-201`
  - `Use april-editor to fix viewport camera instability`
  - `Use april-coordinator to split this into module-owned workstreams`

## 4) Required task lifecycle
1. Select exactly one task card in `agent/sub-agent/<module>/tasks/`.
2. Set `status: in_progress` and update `updated_at`.
3. Implement a mergeable slice and verify with build/test/demo.
   - For generated artifacts: do not edit `AUTO-GENERATED` files directly. Edit source inputs and regenerate in the same change.
4. Set `status: done` and fill `evidence`.

## 5) DocSync gate
- If public API/behavior/contracts changed, update docs in the same change:
  - `agent/sub-agent/<module>/interfaces.md`
  - `agent/sub-agent/<module>/changelog.md`
  - and/or `agent/integration/*.md`
- Validate with: `python agent/script/check-docsync.py`

## 6) Recommended prompts
- `Use april-coordinator. Break this request into module tasks, then execute in order.`
- `Use april-task-lifecycle and complete this task card end-to-end.`
- `Use april-docsync-guard and list exactly which docs were updated.`
