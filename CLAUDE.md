# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (Ninja Debug)
cmake --preset x64-debug

# Build
cmake --build build/x64-debug

# Configure (VS2022)
cmake --preset vs2022

# Build VS2022
cmake --build build/vs2022 --config Debug
```

Build outputs go to `bin/` (executables) and `bin/lib/` (libraries). Shaders are copied to `bin/shader/`.

## Testing

Uses doctest framework. Test executables are in `engine/test/`.

```bash
# Build and run test suite
cmake --build build/x64-debug --target test-suite
./build/bin/test-suite

# Individual test apps
./build/bin/test-profiler-full
./build/bin/test-slang-rhi
```

Target >80% code coverage. Write tests before implementation (TDD).

## Architecture

**Module System**: Each subdirectory of `engine/` with a `manifest.txt` is auto-loaded as CMake target `April_<moduleName>`.

**Core Modules**:
- `core` - Foundation: logging, profiling, math (GLM), file I/O, window abstraction (GLFW)
- `graphics` - RHI (slang-rhi), shader system (Slang), asset management, rendering
- `runtime` - Engine loop, window/device/swapchain init, scene rendering
- `ui` - Dear ImGui integration, ImPlot
- `editor` - Editor application
- `test` - Unit tests and test applications

**Dependency chain**: core → graphics → runtime → ui → editor

**Module Manifest** (`manifest.txt` - INI format):
```ini
[module]
name = mymodule
type = library

[dependencies]
public = core
private = somelib
```

## Code Style (Critical)

**C++23 required**. Use std::format, concepts, ranges, auto.

**Naming**:
- Variables: `camelCase`
- Members: `m_camelCase`
- Pointer params: `p_camelCase`
- Types: `PascalCase`
- Interfaces: `IClassName`
- Files: `kebab-case.hpp/.cpp`
- Namespaces: lowercase, root is `april`

**East Const** (const on right):
```cpp
std::string const& name  // correct
const std::string& name  // wrong
```

**Trailing return types** (always):
```cpp
auto func() -> void;     // correct
void func();             // wrong
```

**Almost Always Auto**:
```cpp
auto message = std::format(...);  // correct
std::string message = ...;        // wrong
```

**Brace initialization**:
```cpp
auto ctx = LogContext{...};  // correct
LogContext ctx(...);         // wrong
```

## Encapsulation Rule (Critical)

Never bypass internal abstractions. If a library (GLFW, ImGui, Slang, Vulkan) is wrapped by an engine class, use the wrapper.

Before including `<vulkan/vulkan.h>`, check if `"rhi/vulkan_types.h"` exists. If a wrapper lacks a feature, extend the wrapper rather than using raw library calls.

## Workflow

Track work in `conductor/plan.md`. Mark tasks `[ ]` → `[~]` → `[x]`.

Commit format: `<type>(<scope>): <description>`
Types: feat, fix, docs, style, refactor, test, chore

You are authorized to execute built binaries (e.g., `build/bin/AprilEngine.exe`) for verification.

## Key Patterns

**Object system**: Use `APRIL_OBJECT(ClassName)` macro for reflection
**Smart pointers**: Use `core::ref<T>` for shared ownership
**Logging**: Use `AP_INFO`, `AP_WARN`, `AP_ERROR` macros
**Graphics commands**: Issue through `CommandContext` acquired from Device
