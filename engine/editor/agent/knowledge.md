# Editor Knowledge

## Pitfalls
- RPC version mismatch between editor and engine
- Updating scene from non-main thread

## Performance
- Avoid recomputing property panels every frame
- Cache selection-driven queries

## Debugging
- Enable RPC logging for desync issues
- Log editor command history

## Platform Notes
- Ensure editor file dialogs obey platform permissions

## Camera Incident (2026-02-08)

### Symptoms
- RMB look-around horizontal direction was reversed at first.
- After inverting displacement/sign to make horizontal rotation feel correct, rotating while moving (`WASD` + RMB drag) can trigger sudden roll-like behavior and camera state corruption.
- Once corrupted, movement directions become unintuitive (for example `W` feels backward, `D` feels left).

### Current Implementation Notes
- `EditorCamera` now maintains its own look-at state (`m_position`, `m_center`, `m_direction`, `m_up`, `m_right`, `m_pitch`, `m_yaw`).
- Viewport sync writes camera direction back into scene transform every frame:
  - read path: `setRotation(transform.localRotation.x, -transform.localRotation.y)`
  - write path: `transform.localRotation = {pitch, -yaw, 0.0f}`
- Scene render camera view uses `inverse(transform.worldMatrix)`, where transform rotation order is Euler XYZ (`Rx -> Ry -> Rz`).

### Confirmed Findings
- The old issue "rotation looks right but movement is reversed" is caused by convention mismatch between editor camera yaw sign and scene transform yaw sign.
- Converting yaw with opposite sign during viewport sync is necessary, otherwise visual direction and movement basis diverge.

### Likely Remaining Root Cause
- Look-around, movement basis update, and scene-transform sync are still coupled through mixed representations (look-at vectors + Euler extraction).
- During continuous rotate+move, this coupling can introduce unstable orientation reconstruction (appears as unexpected roll/jump), especially when direction->Euler->direction roundtrips happen under active input.

### Next Debug Focus
- Keep a single orientation source of truth during active input (avoid roundtrip reconstruction in the same frame).
- Add frame-level debug logs for:
  - `m_direction/m_up/m_right`
  - extracted `{pitch, yaw}`
  - transform `{localRotation.x, localRotation.y, localRotation.z}`
- Verify orthonormality each frame (`dot(right, up)`, `dot(right, direction)`, `dot(up, direction)` near 0; each length near 1).
