# Manifest Guide

Each module directory should contain a `manifest.txt`.
The build system scans only the first-level subdirectories of each module root and skips folders without `manifest.txt`.

Only the INI-style format is supported.

Note: executables are not defined via manifests. Keep app targets in `entry/CMakeLists.txt`.

## 1) Location and Naming
- Location: `<moduleRoot>/<module>/manifest.txt`
- Module roots: configured by `APRIL_MODULE_ROOTS` (defaults to `${CMAKE_SOURCE_DIR}/engine`)
- Target name: `April_<moduleName>` (`module.name` when provided, otherwise the folder name)
- Version: defaults to `0.0.0` if omitted
- Module type: defaults to `STATIC` (using `library` maps to `STATIC`)

You can override the module roots via CMake:
```sh
cmake -DAPRIL_MODULE_ROOTS="E:/repo/engine;E:/repo/tools/modules" ..
```

## 2) INI Format (Supported)
```ini
[module]
name = editor
type = library
version = 2.0.0

[build]
pch = src/editor_pch.hpp
unity_build = true

[dependencies]
public = core, graphics
private = ui

[dependencies.windows]
private = WindowsNativeFileDialog

[defines]
private = IMGUI_USER_CONFIG="april_imgui_config.h"

[codegen]
cmd = python ../../tools/generate_inspectors.py
output = src/generated

[resources]
copy_dir = { "assets/icons" : "editor/icons" }
```

### Section Details

#### [module]
- `name`: used for target naming (`April_<name>`). If omitted, folder name is used.
- `type`: `library` / `static` / `shared`
- `version`: module version string

#### [build]
- `pch`: precompiled header path **relative to the module root**
- `unity_build`: enable unity build (`true/false/1/0`)

#### [dependencies]
- `public` / `private`: dependency list (comma or space separated)
- Resolution order:
  1) if the name is an existing target, use it
  2) otherwise try `April_<name>`
  3) if still missing, pass to the linker (warning emitted)
 - If a module overrides `module.name`, use that name in dependencies.

Tip: choose unique `module.name` values to avoid target name collisions.

#### [dependencies.<platform>]
Platform-specific dependencies. Supported platform keys:
- `windows` / `win32`
- `linux`
- `macos` / `osx` / `darwin`

Example:
```ini
[dependencies.windows]
private = WindowsNativeFileDialog
```

#### [defines] and [defines.<platform>]
Use `public` / `private`. Multiple defines can be separated by commas:
```ini
[defines]
public = SOME_FEATURE=1, USE_FAST_MATH
```

#### [codegen]
- `cmd`: codegen command (runs with module root as working directory)
- `output`: output directory (added to include paths, relative to module root)

#### [resources]
- `copy_dir`: directory mapping in the form:
  `{ "src/path" : "dst/path" }`
  - `src/path` is relative to the module root
  - `dst/path` is relative to `CMAKE_RUNTIME_OUTPUT_DIRECTORY`

## 3) Troubleshooting
1) **PCH not found**
   Ensure `pch` is relative to the module root, or use an absolute path.

2) **Dependency link failures**
   Use the module directory name, or an existing CMake target name.

3) **Resources not copied**
   Missing source directories are skipped with a warning.
   Destination paths are relative to the runtime output directory.
