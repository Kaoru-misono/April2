---
id: EDITOR-CAMERA-001
title: Stabilize editor camera orientation during simultaneous look-around and movement
status: done
owner: codex
priority: p0
deps: []
updated_at: 2026-02-09
evidence: "manual validation passed (user confirmed no roll when vertically dragging from side view around center cube) on 2026-02-09; implementation in engine/editor/source/editor/editor-camera.cpp and engine/editor/source/editor/window/viewport-window.cpp"
---

## Goal
Fix editor camera instability where rotating (RMB look-around) while moving (`WASD`) can produce unexpected roll/jumps and eventually break navigation direction consistency.

## Context
- Horizontal look-around direction has been manually corrected via displacement/sign adjustments.
- Yaw sign conversion between `EditorCamera` and scene transform is currently required in `viewport-window` sync.
- Remaining issue appears when rotate+move happen concurrently.

## Acceptance Criteria
- [x] RMB look-around left/right and up/down are stable and intuitive.
- [x] Holding `W` while rotating does not introduce roll, sudden flips, or basis corruption.
- [x] `W/S/A/D` stay consistent with current view direction after prolonged rotate+move use.
- [x] Scene camera render orientation matches editor camera movement orientation in every frame.

## Test Plan
- manual: open editor viewport, hold RMB and rotate continuously while pressing `W`, `A`, `D`, `S`.
- manual: repeat at low and high pitch angles near clamp limits.
- manual: verify camera entity transform in Inspector does not show erratic rotation spikes.
- optional debug: print frame data for `direction/right/up`, Euler extraction, and transform local rotation.

## Implementation Notes
- Prefer one orientation source of truth per frame under active input.
- Avoid unstable direction->Euler->direction roundtrip while input is being applied.
- Keep scene sync conversion explicit and documented if sign conversion remains necessary.
