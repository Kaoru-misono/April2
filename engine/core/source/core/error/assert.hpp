#pragma once

#include "../log/logger.hpp"

#include <filesystem>
#include <utility>
#include <format>

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
            auto const str = std::string_view{path};
            auto const pos = str.find_last_of("/\\");
            if (pos != std::string_view::npos)
            {
                return str.substr(pos + 1);
            }
            return str;
        }

        // --------------------------------------------------------------------------
        // Assertion Reporters
        // --------------------------------------------------------------------------

        // Overload 1: With custom formatted message
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

        // Overload 2: No custom message (Default)
        inline auto reportAssertionFailure(
            char const* condition,
            char const* file,
            int line
        ) -> void
        {
            auto const filename = getFileName(file);

            AP_CRITICAL("Assertion Failed\n  File: {}:{}\n  Condition: {}",
                filename, line, condition);
        }

        // --------------------------------------------------------------------------
        // Unreachable Reporters
        // --------------------------------------------------------------------------

        // Overload 1: With custom formatted message
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

        // Overload 2: No custom message (Default)
        inline auto reportUnreachable(
            char const* file,
            int line
        ) -> void
        {
            auto const filename = getFileName(file);

            AP_CRITICAL("Unreachable Code Hit\n  File: {}:{}",
                filename, line);
        }
    }
}

// --------------------------------------------------------------------------
// Macros
// --------------------------------------------------------------------------

// AP_ASSERT
// Usage:
// 1. AP_ASSERT(ptr != nullptr);
// 2. AP_ASSERT(ptr != nullptr, "Pointer {} is null", ptrName);
#if AP_ENABLE_ASSERTS
    // Uses __VA_OPT__(,) to handle the trailing comma issue when no extra args are provided
    #define AP_ASSERT(condition, ...) \
        do { \
            if (!(condition)) { \
                ::april::internal::reportAssertionFailure(#condition, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); \
                AP_DEBUGBREAK(); \
            } \
        } while(0)
#else
    #define AP_ASSERT(condition, ...) do { (void)(condition); } while(0)
#endif

// AP_UNREACHABLE
// Usage:
// 1. AP_UNREACHABLE();
// 2. AP_UNREACHABLE("Unknown enum value: {}", value);
// This is active even in Release builds to signal optimization hints or critical logic errors.
#define AP_UNREACHABLE(...) \
    do { \
        ::april::internal::reportUnreachable(__FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); \
        AP_DEBUGBREAK(); \
        std::unreachable(); \
    } while(0)