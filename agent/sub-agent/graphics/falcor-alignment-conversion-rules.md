# Falcor Alignment Conversion Rules (Graphics)

Use this document when the user requests Falcor-first implementation for graphics/material architecture.

## 1. Single Source of Truth
- Treat Falcor as the architecture source of truth for aligned classes/files.
- Keep function/member count, order, grouping, and visibility (`public/protected/private`) aligned with Falcor.
- Do not invent extra APIs/helpers not present in Falcor for the aligned class.

## 2. April Naming Override (Mandatory)
Even when copying Falcor structure, keep April naming rules:
- Member variables must use `m_` prefix.
- Pointer/smart-pointer parameters must use `p_` prefix.
- Local variables must use `camelCase`.

Examples:
- `mData` -> `m_data`
- `mAlphaRange` -> `m_alphaRange`
- `mpDefaultSampler` -> `m_pDefaultSampler`
- `pDevice` -> `p_device`

## 3. .hpp/.cpp Placement
- If Falcor implements a method in `.cpp`, implement it in `.cpp`.
- Do not move non-trivial function bodies into headers.
- Keep templates and unavoidable inline-only utilities in headers.

## 4. API Fidelity
- Preserve function signature semantics (inputs/outputs/constness/virtual/override), while applying April naming conventions.
- If a Falcor method exists, do not replace it with a custom method name.
- If a Falcor method does not exist, do not add an equivalent helper unless explicitly requested and documented.

## 5. Review Checklist (Per File)
- [ ] Function list matches Falcor (count/order/visibility).
- [ ] Member list matches Falcor (count/order/visibility).
- [ ] No extra non-Falcor APIs introduced.
- [ ] All members use `m_` naming.
- [ ] All pointer/smart-pointer params use `p_` naming.
- [ ] `.cpp`/`.hpp` placement follows Falcor layout.
