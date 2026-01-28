#pragma once

#include "log/logger.hpp"       // For logging integration

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <format>
#include <span>
#include <utility>
#include <type_traits>

namespace april
{
    // --------------------------------------------------------------------------
    // ADL Lookup Mechanism
    // --------------------------------------------------------------------------
    // We use Argument Dependent Lookup (ADL) to find the info struct declared
    // by the macros without polluting the global namespace.
    template <typename T>
    using EnumInfo = decltype(aprilFindEnumInfoADL(std::declval<T>()));

    // Concept to check if an enum has registered info
    template <typename T>
    concept EnumReflectable = requires {
        { EnumInfo<T>::items() } -> std::convertible_to<std::span<const std::pair<T, std::string_view>>>;
    };

    // --------------------------------------------------------------------------
    // Core Conversions
    // --------------------------------------------------------------------------

    /**
     * Convert an enum value to its string representation.
     * Aborts/Critical logs if the value is not registered.
     */
    template <EnumReflectable T>
    [[nodiscard]] constexpr auto enumToString(T value) -> std::string_view
    {
        auto const items = EnumInfo<T>::items();
        auto const it = std::ranges::find_if(items, [value](auto const& item) {
            return item.first == value;
        });

        if (it == items.end()) [[unlikely]]
        {
            // Fallback for flags or unknown values?
            // Falcor throws, we use your critical log system.
            // Converting value to int for logging requires casting.
            using U = std::underlying_type_t<T>;
            AP_CRITICAL("Invalid enum value '{}' for type '{}'", static_cast<U>(value), typeid(T).name());
            return "<Invalid Enum>";
        }
        return it->second;
    }

    /**
     * Convert a string to an enum value.
     * Aborts/Critical logs if the name is not found.
     */
    template <EnumReflectable T>
    [[nodiscard]] constexpr auto stringToEnum(std::string_view name) -> T
    {
        auto const items = EnumInfo<T>::items();
        auto const it = std::ranges::find_if(items, [name](auto const& item) {
            return item.second == name;
        });

        if (it == items.end()) [[unlikely]]
        {
            AP_CRITICAL("Invalid enum name '{}' for type '{}'", name, typeid(T).name());
            return {}; // Return default construction (or UB if we didn't abort)
        }
        return it->first;
    }

    /**
     * Check if a string corresponds to a valid enum value.
     */
    template <EnumReflectable T>
    [[nodiscard]] constexpr auto enumHasValue(std::string_view name) -> bool
    {
        auto const items = EnumInfo<T>::items();
        return std::ranges::any_of(items, [name](auto const& item) {
            return item.second == name;
        });
    }

    // --------------------------------------------------------------------------
    // Flag (Bitmask) Conversions
    // --------------------------------------------------------------------------

    /**
     * Convert a flag enum value (bitmask) to a list of strings.
     * Checks bits against registered enum values.
     */
    template <EnumReflectable T>
    [[nodiscard]] auto flagsToStringList(T flags) -> std::vector<std::string>
    {
        std::vector<std::string> list;
        auto const items = EnumInfo<T>::items();

        // We need a mutable copy to track which bits we've handled
        T remainingFlags = flags;

        for (auto const& [val, name] : items)
        {
            // Skip zero values (None) to avoid infinite loops or bad logic
            using U = std::underlying_type_t<T>;
            if (static_cast<U>(val) == 0) continue;

            // Check if the bit(s) in 'val' are set in 'flags'
            // We use the boolean operator logic usually provided by enum-flags.hpp
            // If not available, we use standard bitwise logic:
            if ((remainingFlags & val) == val)
            {
                list.emplace_back(name);
                // Clear these bits from our tracker
                remainingFlags &= ~val;
            }
        }

        // If bits remain that weren't matched to a name
        if (static_cast<std::underlying_type_t<T>>(remainingFlags) != 0)
        {
            AP_WARN("Unregistered bits remaining in flags value for type '{}'", typeid(T).name());
        }

        return list;
    }

    /**
     * Convert a list of strings to a flag enum value.
     */
    template <EnumReflectable T>
    [[nodiscard]] auto stringListToFlags(std::vector<std::string> const& list) -> T
    {
        T flags = static_cast<T>(0);
        for (auto const& name : list)
        {
            flags |= stringToEnum<T>(name);
        }
        return flags;
    }
} // namespace april

// --------------------------------------------------------------------------
// Registration Macros
// --------------------------------------------------------------------------

/**
 * Define enum information.
 * Usage:
 * AP_ENUM_INFO(MyEnum, {
 * { MyEnum::Val1, "Val1" },
 * { MyEnum::Val2, "Val2" }
 * })
 */
#define AP_ENUM_INFO(T, ...) \
    struct T##_info { \
        static constexpr std::pair<T, std::string_view> data[] = __VA_ARGS__; \
        static constexpr std::span<const std::pair<T, std::string_view>> items() { \
            return std::span(data); \
        } \
    };

/**
 * Register the enum lookup function via ADL.
 * Must be placed in the same namespace as the enum T.
 * * Usage: AP_ENUM_REGISTER(MyNamespace::MyEnum);
 */
#define AP_ENUM_REGISTER(T) \
    constexpr T##_info aprilFindEnumInfoADL(T) noexcept { return T##_info{}; }


// --------------------------------------------------------------------------
// std::formatter Specialization
// --------------------------------------------------------------------------

/**
 * Enables automatic formatting of registered enums in std::format / std::print.
 * Example: std::print("Value is: {}", MyEnum::A); -> "Value is: A"
 */
template <typename T>
requires april::EnumReflectable<T>
struct std::formatter<T> : std::formatter<std::string_view>
{
    auto format(T const& e, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(april::enumToString(e), ctx);
    }
};
