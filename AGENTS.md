# Repository Guidelines

## Project Structure & Module Organization
- `engine/` houses C++ modules (`core`, `graphics`, `runtime`, `ui`, `editor`, `scene`, `asset`, `test`). Each module with a `manifest.txt` becomes a CMake target named `April_<module>`.
- `entry/` contains application entry points (for example, `editor/` and `game/`).
- `content/` stores assets (`material/`, `model/`, `texture/`, `library/`).
- `cmake/`, `CMakeLists.txt`, and `CMakePresets.json` define the build system.
- Build outputs go to `bin/` (executables), `bin/lib/` (libraries), and `bin/shader/` (copied shaders).

## Build, Test, and Development Commands
- `cmake --preset x64-debug` configures a Ninja Debug build in `build/x64-debug`.
- `cmake --build build/x64-debug` builds the Ninja Debug targets.
- `cmake --preset vs2022` configures a Visual Studio 2022 build in `build/vs2022`.
- `cmake --build build/vs2022 --config Debug` builds the VS2022 Debug targets.
- `cmake --build build/x64-debug --target test-suite` builds the doctest test suite.
- `./build/bin/test-suite` runs the full suite; individual tests include `./build/bin/test-profiler-full` and `./build/bin/test-slang-rhi`.

## Coding Style & Naming Conventions
- C++23 is required; prefer `std::format`, concepts, ranges, and `auto` for local variables.
- Use east-const (`std::string const&`), trailing return types (`auto f() -> void`), and brace initialization (`Type t{}`).
- Naming: variables `camelCase`, members `m_camelCase`, pointer params `p_camelCase`, types `PascalCase`, interfaces `IClassName`, files `kebab-case.hpp/.cpp`, namespaces lowercase with root `april`.
- Indentation is 4 spaces with braces on the next line (Allman style). No formatter is enforced; match the surrounding file.

## Testing Guidelines
- Tests use the doctest framework and live in `engine/test/`.
- Test sources are named `test-*.cpp` (for example, `test-asset.cpp`).
- Target >80% coverage and write tests before implementation when feasible.

## Commit & Pull Request Guidelines
- Commit messages are short and imperative; module prefixes like `scene:` and `WIP:` tags are common.
- PRs should include a summary, the exact build/test commands run (with preset), and linked issues when applicable.
- Include screenshots or short clips for editor or rendering changes.

## Architecture Notes
- Module dependency chain: `core` → `graphics` → `runtime` → `ui` → `editor`.
- Do not bypass engine wrappers for third-party libs (GLFW, ImGui, Slang, Vulkan); extend wrappers instead.
