---
name: april-cross-module-contract
description: Use when implementation requires another module. Formalizes request-first contracts so work can continue without tight coupling.
---

# April Cross-Module Contract

## Objective
Enable parallel development across modules without blocking.

## Procedure
1. Capture dependency request in the target module `plan/` or task file.
2. Specify exact contract:
   - symbols/types and signatures
   - ownership/lifetime/threading constraints
   - error behavior and fallback handling
3. Continue local implementation against the documented contract.
4. Add verification notes describing assumptions.
5. Create follow-up task if integration validation is pending.

## Contract Template
- Requester module:
- Target module:
- API/contract shape:
- Threading expectations:
- Error and recovery semantics:
- Validation command:

## Rules
- No hidden cross-module assumptions.
- Contract changes require integration doc updates.
- Keep requests specific and implementation-agnostic.
