#pragma once

#include <type_traits>

template <typename T>
concept enum_type = std::is_enum_v<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>;

// This macro defines bitwise operators for an enum type that already is an flag.
#define AP_ENUM_CLASS_OPERATORS(ENUM) \
    inline constexpr auto operator |= (ENUM& lhs, ENUM rhs) noexcept -> ENUM& { using underlying_type = std::underlying_type_t<ENUM>; return lhs = (ENUM) (underlying_type(lhs) | underlying_type(rhs)); } \
    inline constexpr auto operator &= (ENUM& lhs, ENUM rhs) noexcept -> ENUM& { using underlying_type = std::underlying_type_t<ENUM>; return lhs = (ENUM) (underlying_type(lhs) & underlying_type(rhs)); } \
    inline constexpr auto operator ^= (ENUM& lhs, ENUM rhs) noexcept -> ENUM& { using underlying_type = std::underlying_type_t<ENUM>; return lhs = (ENUM) (underlying_type(lhs) ^ underlying_type(rhs));} \
    inline constexpr auto operator |  (ENUM lhs, ENUM rhs) noexcept -> ENUM { using underlying_type = std::underlying_type_t<ENUM>; return (ENUM) (underlying_type(lhs) | underlying_type(rhs)); } \
    inline constexpr auto operator &  (ENUM lhs, ENUM rhs) noexcept -> ENUM { using underlying_type = std::underlying_type_t<ENUM>; return (ENUM) (underlying_type(lhs) & underlying_type(rhs)); } \
    inline constexpr auto operator ^  (ENUM lhs, ENUM rhs) noexcept -> ENUM { using underlying_type = std::underlying_type_t<ENUM>; return (ENUM) (underlying_type(lhs) ^ underlying_type(rhs)); } \
    inline constexpr auto operator !  (ENUM e) noexcept -> bool { using underlying_type = std::underlying_type_t<ENUM>; return !underlying_type(e); } \
    inline constexpr auto operator ~  (ENUM e) noexcept -> ENUM { using underlying_type = std::underlying_type_t<ENUM>; return (ENUM) ~underlying_type(e); } \
    static_assert(std::is_unsigned_v<std::underlying_type_t<ENUM>>, "Enum must use unsigned underlying type.")


template <enum_type ENUM>
constexpr auto enum_has_all_flags(ENUM flags, ENUM contains) -> bool
{
    using underlying_type = std::underlying_type_t<ENUM>;
    return (underlying_type(flags) & underlying_type(contains)) == underlying_type(contains);
}

template <enum_type ENUM>
constexpr auto enum_has_any_flags(ENUM flags, ENUM contains) -> bool
{
    using underlying_type = std::underlying_type_t<ENUM>;
    return (underlying_type(flags) & underlying_type(contains)) != 0;
}

template <enum_type ENUM>
constexpr auto enum_add_flags(ENUM& flags, ENUM flags_to_add) -> void
{
    using underlying_type = std::underlying_type_t<ENUM>;
    flags = (ENUM) (underlying_type(flags) | underlying_type(flags_to_add));
}

template <enum_type ENUM>
constexpr auto enum_remove_flags(ENUM& flags, ENUM flags_to_remove) -> void
{
    using underlying_type = std::underlying_type_t<ENUM>;
    flags = (ENUM) (underlying_type(flags) & ~underlying_type(flags_to_remove));
}
