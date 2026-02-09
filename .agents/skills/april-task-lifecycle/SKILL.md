---
name: april-task-lifecycle
description: Use when implementing or updating engineering work items in April2. Enforces one-task-in-progress workflow, task status updates, and evidence capture.
---

# April Task Lifecycle

Use this skill whenever work maps to a tracked task card.

## Inputs
- target module (`core`, `graphics`, `runtime`, `ui`, `editor`, `scene`, `asset`, `test`)
- concrete objective from user request

## Procedure
1. Locate the module task source in `agent/sub-agent/<module>/`.
2. Select one task card for active execution.
3. Move task to `in_progress` before coding.
4. Run `python .agents/skills/april-task-lifecycle/check-task-card.py --file <task-path>` after each status update.
5. Implement the smallest mergeable slice.
6. Validate with at least one build/test/demo command.
7. Create one commit dedicated to the completed task, including both code and task-card updates.
8. Use the task id in the commit message (for example `GRAPHICS-MATERIAL-201: ...`).
9. Mark task `done` and record evidence with verification command(s) and result notes.
10. After all tasks in the execution batch are done, update task tracker doc with `task id -> commit id` mappings.
11. If additional work is discovered, create a new task card.

## Rules
- Never keep multiple active tasks for one sub-agent execution.
- Do not silently change public contracts.
- Keep tasks mergeable and bounded to clear files.
- Do not manually edit files marked `AUTO-GENERATED`; update generator inputs and regenerate.
- Every completed task must have a single task-scoped commit, and commit id tracking belongs in task tracker doc (not task `evidence`).
- A task status update is not complete until the task card passes `check-task-card.py`.

## Outputs
- updated task card status
- evidence section with verification command
- optional follow-up task cards
