# Specification: Core Profile and UI Element Reformatting

## Overview
This track aims to align the codebase for the Core Profile and UI Element modules with the project's C++ code style guidelines. The focus is on modernization and consistency using non-breaking changes.

## Scope
The following directories and files are included:
- **Core Profile:** `engine/core/source/core/profile/` (`profiler.hpp/cpp`, `timers.hpp/cpp`)
- **UI Element:** `engine/ui/source/ui/element/` (`element-logger.hpp/cpp`, `element-profiler.hpp/cpp`)
- **Tests:** `engine/test/test-profiler.cpp` and other related test files in `engine/test/`.

## Requirements

### 1. Modern C++ Syntax (Non-Breaking)
Apply the following rules as defined in `conductor/code_styleguides/general.md`:
- **Trailing Return Types:** All function declarations must use `auto ... -> type`.
- **East Const:** Use `type const&` or `type const*` instead of `const type&`.
- **Almost Always Auto (AAA):** Use `auto` for local variable declarations.
- **Brace Initialization:** Use `{}` for object construction and initialization where applicable.

### 2. Constraints
- **NO File Renaming:** Filenames must remain exactly as they are.
- **NO Symbol Renaming:** Class names, member variables (including those without `m_`), and function names must remain unchanged to avoid breaking dependencies.
- **Namespace Consistency:** Ensure code is correctly wrapped in the `april` namespace (or sub-namespaces) but do not change the namespace names themselves.

## Acceptance Criteria
- [ ] All specified files pass compilation.
- [ ] All specified files strictly follow trailing return type syntax.
- [ ] All specified files use East Const syntax.
- [ ] Local variables in these files use `auto` where appropriate.
- [ ] Existing functionality remains unchanged (verified by tests).

## Out of Scope
- Renaming files to kebab-case.
- Renaming members to include `m_` prefix.
- Refactoring logic or improving performance (formatting only).