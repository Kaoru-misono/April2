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
4. Implement the smallest mergeable slice.
5. Validate with at least one build/test/demo command.
6. Mark task `done` and record evidence (commit hash/PR and command used).
7. If additional work is discovered, create a new task card.

## Rules
- Never keep multiple active tasks for one sub-agent execution.
- Do not silently change public contracts.
- Keep tasks mergeable and bounded to clear files.
- Do not manually edit files marked `AUTO-GENERATED`; update generator inputs and regenerate.

## Outputs
- updated task card status
- evidence section with verification command
- optional follow-up task cards
