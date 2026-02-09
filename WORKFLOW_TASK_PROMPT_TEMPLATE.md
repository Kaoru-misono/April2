# Workflow Task Prompt Template

Use the template below to start a task with the current Skill + sub-agent workflow.

```text
Use april-<module> to execute <task-card-path>.
Before coding, set status to in_progress and updated_at to today.
Implement the requested change within module ownership boundaries.
Never hand-edit `AUTO-GENERATED` files; update source/generator inputs and regenerate.
After changes, run relevant build/test/manual verification.
When a task is complete, create one commit that contains both code changes and task-card updates for that task.
Use the task id in commit message (for example `GRAPHICS-MATERIAL-201: ...`).
`evidence` should record verification details; commit id tracking is done in the task tracker document.
If public API/contract changed, update docs with docsync guard.
```

## Examples

- `Use april-editor to execute agent/sub-agent/editor/tasks/EDITOR-CAMERA-001.md ...`
- `Use april-graphics to execute agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-201-material-system-descriptor-tables.md ...`
