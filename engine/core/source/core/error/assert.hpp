#pragma once

#include "../log/logger.hpp"

#include <filesystem>
#include <utility>

// --------------------------------------------------------------------------
// Platform specific debug break
// --------------------------------------------------------------------------
#if defined(_MSC_VER)
    #define AP_DEBUGBREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
    #define AP_DEBUGBREAK() __builtin_trap()
#else
    #define AP_DEBUGBREAK() std::abort()
#endif

// --------------------------------------------------------------------------
// Configuration
// --------------------------------------------------------------------------
// Automatically enable asserts in Debug builds, or if explicitly requested
#if !defined(AP_ENABLE_ASSERTS)
    #if defined(_DEBUG) || defined(DEBUG)
        #define AP_ENABLE_ASSERTS 1
    #else
        #define AP_ENABLE_ASSERTS 0
    #endif
#endif

namespace april
{
    namespace internal
    {
        // Helper to extract just the filename from a full path
        inline auto getFileName(char const* path) -> std::string_view
        {
            auto const fsPath = std::filesystem::path(path);
            // We return the generic string of the filename
            // Note: std::filesystem::path::filename() returns a path, we need a string view mechanism
            // For efficiency in macros, a simple string search might be faster,
            // but filesystem is safer for cross-platform separators.
            // Returning the pointer to the filename part of the original string to avoid allocation:
            auto const str = std::string_view{path};
            auto const pos = str.find_last_of("/\\");
            if (pos != std::string_view::npos)
            {
                return str.substr(pos + 1);
            }
            return str;
        }

        template <typename... Args>
        auto reportAssertionFailure(
            char const* condition,
            char const* file,
            int line,
            std::format_string<Args...> fmt,
            Args&&... args
        ) -> void
        {
            auto const message = std::format(fmt, std::forward<Args>(args)...);
            auto const filename = getFileName(file);

            AP_CRITICAL("Assertion Failed: {}\n  File: {}:{}\n  Condition: {}",
                message, filename, line, condition);
        }

        template <typename... Args>
        auto reportUnreachable(
            char const* file,
            int line,
            std::format_string<Args...> fmt,
            Args&&... args
        ) -> void
        {
            auto const message = std::format(fmt, std::forward<Args>(args)...);
            auto const filename = getFileName(file);

            AP_CRITICAL("Unreachable Code Hit: {}\n  File: {}:{}",
                message, filename, line);
        }
    }
}

// --------------------------------------------------------------------------
// Macros
// --------------------------------------------------------------------------

// AP_ASSERT
// Usage: AP_ASSERT(ptr != nullptr, "Pointer cannot be null");
#if AP_ENABLE_ASSERTS
    #define AP_ASSERT(condition, ...) \
        do { \
            if (!(condition)) { \
                ::april::internal::reportAssertionFailure(#condition, __FILE__, __LINE__, __VA_ARGS__); \
                AP_DEBUGBREAK(); \
            } \
        } while(0)
#else
    #define AP_ASSERT(condition, ...) do { (void)(condition); } while(0)
#endif

// AP_UNREACHABLE
// Usage: AP_UNREACHABLE("This switch case should not happen");
// This is active even in Release builds to signal optimization hints or critical logic errors.
#define AP_UNREACHABLE(...) \
    do { \
        ::april::internal::reportUnreachable(__FILE__, __LINE__, __VA_ARGS__); \
        AP_DEBUGBREAK(); \
        std::unreachable(); \
    } while(0)
