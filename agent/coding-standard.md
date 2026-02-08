# C++ Code Style Guidelines

## 1. Language Standard & Modern Features
- Target C++23.
- Prefer modern features such as `std::format`, `std::source_location`, concepts, ranges, and modules (when applicable).

## 2. Naming Conventions
### 2.1 Variables & Members
- Local variables: `camelCase`
```cpp
auto message = std::format(...);
```
- Member variables: `m_` + `camelCase`
```cpp
std::string m_name{};
LogConfig m_config{};
```
- Pointer or smart pointer parameters: `p_` + `camelCase`
```cpp
auto addSink(std::shared_ptr<ILogSink> p_sink) -> void;
```
- Globals/statics: avoid. If necessary, use `g_` or `s_`.

### 2.2 Types
- Classes/structs: `PascalCase`
```cpp
class Logger {};
struct LogContext {};
```
- Interfaces: `I` + `PascalCase`
```cpp
class ILogSink {};
```
- Templates: use `template <...>` with a space before `<`.
```cpp
template <typename Value>
```

### 2.3 Files
- Filenames: `kebab-case` (lowercase with hyphens).
- Extensions: `.hpp` for headers, `.cpp` for implementations.

## 3. Function Declarations & Formatting
- Always use trailing return types.
```cpp
auto getName() const -> std::string const&;
auto trace(...) -> void;
```
- Keep short inline definitions on one line; otherwise use Allman style braces.
```cpp
auto func() -> void { ... }

auto func() -> void
{
    ...
}
```
- For long parameter lists, break onto new lines and align.
```cpp
auto func(
    int a,
    int b,
    int c
) -> void
{
    ...
}
```
- Lambda formatting follows the same brace style.
```cpp
auto lambda = []() -> void
{
    ...
};
```

## 4. Parameter Passing
- Pass by value for cheap types (e.g., `int`, `bool`, small enums).
- Pass by `const&` for non-trivial types.
```cpp
Logger(std::string const& name);
```
- Use rvalue references (`T&&`) to transfer ownership.

## 5. Variable Declarations (AAA)
- Almost Always Auto: prefer `auto` for local variables.
```cpp
auto message = std::format(fmt, ...);
auto sinksCopy = std::vector<std::shared_ptr<ILogSink>>{};
auto* pRawPtr = getRawPointer();
```
- Prefer brace initialization.
```cpp
auto context = LogContext{...};
LogConfig const& config = {};
```

## 6. Const Correctness (East Const)
- Place `const` to the right of the type.
```cpp
std::string const& name;
LogConfig const& config;
auto func() const -> void;
```

## 7. Macros & Preprocessor
- Minimize macro usage; prefer `constexpr`, `const`, or `inline`.
- Allowed: logging macros (e.g., `AP_INFO`) and platform guards.
- Macro naming: `UPPER_CASE_WITH_UNDERSCORES`.
- Use `#pragma once` for header guards.

## 8. Namespaces
- Use lowercase namespaces; root namespace is `april`.
```cpp
namespace april
{
    // code
}
```
