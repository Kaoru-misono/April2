# Workflow Task Prompt Template

Use the template below to start a task with the current Skill + sub-agent workflow.

```text
Use april-<module> to execute <task-card-path>.
Before coding, set status to in_progress and updated_at to today.
Implement the requested change within module ownership boundaries.
After changes, run relevant build/test/manual verification, update evidence, then set status to done.
If public API/contract changed, update docs with docsync guard.
```

## Examples

- `Use april-editor to execute agent/sub-agent/editor/tasks/EDITOR-CAMERA-001.md ...`
- `Use april-graphics to execute agent/sub-agent/graphics/tasks/GRAPHICS-MATERIAL-201-material-system-descriptor-tables.md ...`
