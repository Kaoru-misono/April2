---
name: april-module-execution
description: Use when executing implementation work in a specific April2 engine module. Applies module role boundaries and knowledge notes before coding.
---

# April Module Execution

## Inputs
- module name
- user objective

## Procedure
1. Read `agent/sub-agent/<module>/role.md`.
2. Read `agent/sub-agent/<module>/knowledge.md`.
3. Scan relevant `interfaces.md` before modifying shared APIs.
4. Implement change within module ownership boundaries.
5. If touching another module, invoke the cross-module contract workflow.
6. Update module `changelog.md` when public behavior changes.

## Quality Bar
- follow C++23 + repository style
- preserve wrapper abstractions around third-party libraries
- include build/test evidence for the modified path

## Outputs
- code changes scoped to module responsibilities
- task and docs updated with evidence
