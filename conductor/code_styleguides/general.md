# C++ Code Style Guidelines
## 1. Modern C++ Standards
- Version: The project targets C++23.
- Features: actively use modern features such as std::format, std::source_location, concepts, ranges, and modules (where applicable).

## 2. Naming Conventions
### 2.1 Variables & Members
#### Local Variables: camelCase
```
C++
auto message = std::format(...);
```
#### Member Variables: m_ prefix + camelCase
```
C++
std::string m_name{};
LogConfig m_config{};
```
#### Pointer/Smart Pointer Arguments: p_ prefix + camelCase This helps distinguish pointer semantics from references or values immediately.
```
C++
auto addSink(std::shared_ptr<ILogSink> p_sink) -> void;
Global/Static Variables: Avoid where possible. If necessary, use g_ or s_ prefix.
```
### 2.2 Types
#### Classes/Structs: PascalCase
```
C++
class Logger
{
    //...
};
struct LogContext
{
    //...
};
```
#### Interfaces: I prefix + PascalCase
```
```
C++
class ILogSink { ... };
#### Templates: space between template and <>
```
C++
template <typename Value>
```

#### Functions:
in header file, if function body less than one line:
```
C++
auto func() -> void { ... }
```
in source file or function body more than one line:
```
C++
auto func() -> void
{

}
```
If need to break line because of length:
```
C++
auto func(
    void a,
    void b,
    void c
) -> void
{
    ...
}
```

### 2.3 Files
#### Filenames: kebab-case (lowercase with hyphens).

use log-sink.hpp
not LogSink.hpp, log_sink.hpp

#### Extensions: .hpp for headers, .cpp for implementation.

## 3. Function Declarations
### 3.1 Trailing Return Types
ALL functions must use trailing return syntax (auto ... -> type). This ensures consistency and improves readability for complex return types.
Do:
```
C++
auto getName() const -> std::string const&;
auto trace(...) -> void;
```
Don't:
```
C++
const std::string& getName() const;
void trace(...);
```
### 3.2 Parameter Passing
Pass by Value: For primitive types (int, bool, float) or cheap-to-copy enums.
Pass by Const Reference: For non-primitive types that are not modified.
```
C++
Logger(std::string const& name); // East Const style
Pass by R-value Reference: When ownership transfer is intended (Args&&... args).
```
## 4. Variable Declarations (AAA)
### 4.1 Almost Always Auto
Use auto for local variable declarations to enforce initialization and prevent implicit conversions. The type should be on the right-hand side.
Do:
```
C++
auto message = std::format(fmt, ...);
auto sinksCopy = std::vector<std::shared_ptr<ILogSink>>{};
auto* pRawPtr = getRawPointer(); // Keep * for raw pointers to be explicit
```
Don't:
```
C++
std::string message = std::format(fmt, ...);
std::vector<std::shared_ptr<ILogSink>> sinksCopy;
```
### 4.2 Initialization
Prefer brace initialization {} (Uniform Initialization) over parentheses () or assignment = for object construction.
Do:
```
C++
auto context = LogContext{ ... };
LogConfig const& config = {};
```
## 5. Const Correctness (East Const)
Place the const qualifier to the right of the type it modifies ("East Const"). This rule applies to variables, references, and methods.
Logic: T const reads as "T is constant". T const& reads as "Reference to T constant".
Do:
```
C++
std::string const& name
LogConfig const& config
auto func() const -> void
```
Don't:
```
C++
const std::string& name
const LogConfig& config
```
## 6. Macros & Preprocessor
Usage: Minimize macro usage. Prefer constexpr, const, or inline functions.
Exceptions: Logging macros (AP_INFO, etc.) or platform-specific compile guards are permitted.
Naming: Macros must be UPPER_CASE_WITH_UNDERSCORES.
Pragma: Use #pragma once for header guards.

## 7. Namespace
Namespaces should be lowercase.
The root namespace is april.
C++
namespace april
{
    // code
}

## Encapsulation Hierarchy (CRITICAL)
- Do NOT bypass internal abstractions: If a third-party library (e.g., GLFW, ImGui, Slang) is wrapped by an internal engine class, you MUST use the internal class.
- Check before Import: Before including a raw library header (e.g., #include <vulkan/vulkan.h>), verify if an engine header (e.g., #include "rhi/vulkan_types.h") already exists.
- Extension Pattern: If the internal wrapper lacks a specific feature you need, do not revert to raw library usage elsewhere. Instead, extend the wrapper to support the new feature, then use the updated wrapper.
