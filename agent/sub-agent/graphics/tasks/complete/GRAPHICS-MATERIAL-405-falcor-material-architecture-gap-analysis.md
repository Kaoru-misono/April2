---
id: GRAPHICS-MATERIAL-405
title: Deep analysis of Falcor vs April material architecture and gap report
status: done
owner: codex
priority: p1
deps: [GRAPHICS-MATERIAL-404]
updated_at: 2026-02-09
evidence: "Produced detailed architecture report at `agent/sub-agent/graphics/research/falcor-vs-april-material-architecture.md` with Falcor/April source-backed comparison, 12 categorized gaps, and phased roadmap. Validation: source inspection across Falcor (`external-repo/Scene/Material/*`) and April material/scene/editor paths, then consistency review against existing GRAPHICS-MATERIAL task outputs."
---

## Goal
Produce a detailed architecture analysis comparing Falcor's material system with April's current material system, identify concrete weaknesses in April's design, and define prioritized improvement points.

## Scope
- Analyze Falcor material architecture from source placed at repository root (data model, type system, shading path, descriptor/resource model, extensibility, cache strategy, tooling).
- Analyze April current material architecture across host + shader + scene/editor integration.
- Perform one-to-one capability mapping (Falcor vs April) and identify missing/fragile areas.
- Produce an actionable improvement plan with phases, dependencies, risk, and expected impact.

## Deliverables
- Architecture comparison report with diagrams/tables.
- Gap list grouped by: correctness, extensibility, performance, tooling/debuggability, maintainability.
- Improvement roadmap (short/mid/long term) with implementation order and validation strategy.

## Acceptance Criteria
- [x] Report references specific Falcor and April source locations for each claim.
- [x] At least 10 concrete architecture gaps are identified and categorized.
- [x] Each gap has a proposed fix, estimated complexity, and verification method.
- [x] Roadmap includes migration sequencing that avoids breaking current runtime path.

## Test Plan
- manual: peer review report with graphics module owner.
- manual: validate proposed roadmap against existing GRAPHICS-MATERIAL tasks and module boundaries.

## Expected Files
- `agent/sub-agent/graphics/research/falcor-vs-april-material-architecture.md`
- `agent/sub-agent/graphics/changelog.md` (if architecture-level guidance is updated)
