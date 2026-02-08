# Global Threading Model (cross-module contract)

## Goal
Define where APIs may be called to prevent hidden data races and stalls.

## Threads & Responsibilities
- **Main Thread**: gameplay logic, scene/world, editor commands, dispatch resource requests
- **Render Thread (optional)**: submit rendering commands / drive RHI
- **Worker Threads**: async IO, asset import, job system

## Rules
1. Every public API must declare thread requirements (Main-only / Render-only / Any-thread).
2. Never block on IO from the Render Thread.
3. Sync points must be explicit: any API that may flush/sync GPU or wait must state so in `interfaces.md`.

## Suggested Tick Order
- Pump OS events
- Apply Editor RPC / UI commands
- World update
- Render prepare (build command list)
- Submit render
- Present
