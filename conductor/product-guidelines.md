# Product Guidelines - April Engine

## Development Philosophy
- **Modern C++ Excellence:** Strictly adhere to **C++23 standards**, utilizing Modules, Concepts, and Ranges to build a type-safe, performant, and future-proof engine.
- **Snake_Case Convention:** Use **snake_case** for internal code naming (variables, functions).
- **Kebab-Case File Naming:** Use **kebab-case** (hyphens) for file and directory names (e.g., `file-dialog.cpp`).
- **Trailing Return Types:** Prefer the use of trailing return types (`auto func() -> type`) for all function declarations to maintain consistency and improve readability.
- **Minimalist Code Style:** Favor clean, expressive code with a heavy reliance on `auto` where appropriate. Comments should be kept to a minimum, used only to explain the "why" of complex logic rather than the "what".

## Quality Assurance & Testing
- **Test-Driven Development (TDD):** Employ a TDD approach for all core components, especially the RHI. Unit tests must be written alongside or before feature implementation/fixes to ensure high coverage and prevent regressions.
- **Stability First:** Prioritize a stable and well-tested core. No architectural changes or bug fixes should be considered complete without corresponding automated tests.

## Workflow & Collaboration
- **Iterative Development:** Focus on small, self-contained, and atomic commits. Each commit should represent a single logical change or fix and must include relevant tests.
- **Feature Stability:** Ensure that the main branch remains stable. Complex overhauls (like the RHI) should be broken down into manageable, testable iterations.

## User Interface & Experience (Developer Tools)
- **Minimalist & Functional UI:** All internal tools and debug overlays (e.g., Dear ImGui integrations) must prioritize utility and clear data presentation over decorative aesthetics.
- **Dark Theme Standard:** Utilize a consistent dark theme for all developer-facing interfaces to reduce eye strain during long development sessions.
