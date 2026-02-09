# Task Card Schema

All task cards under `agent/sub-agent/*/tasks/*.md` must use this frontmatter schema.

## Required frontmatter
```yaml
---
id: <MODULE-NNN>
title: <one-line goal>
status: todo            # todo | in_progress | blocked | done
owner: codex            # or team handle
priority: p1            # p0 | p1 | p2 | p3
deps: []                # array of task ids
updated_at: YYYY-MM-DD
evidence: ""            # commit hash / PR / verification notes
---
```

## Required sections
- `## Goal`
- `## Acceptance Criteria`
- `## Test Plan`

## Optional sections
- `## Scope`
- `## Context`
- `## Expected Files`
- `## Notes`
- `## Out of Scope`

## Rules
- One task card = one mergeable unit of work.
- Keep acceptance criteria testable and specific.
- Update `status`, `updated_at`, and `evidence` as work progresses.
