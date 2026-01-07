#pragma once

#define AP_PINNED(STRUCT) \
    STRUCT(STRUCT const&) = delete; \
    auto operator = (STRUCT const&) -> STRUCT& = delete; \
    STRUCT(STRUCT&&) = delete; \
    auto operator = (STRUCT&&) -> STRUCT& = delete; \
    static_assert(true)

#define AP_MOVE_ONLY(STRUCT) \
    STRUCT(STRUCT const&) = delete; \
    auto operator = (STRUCT const&) -> STRUCT& = delete; \
    static_assert(true)

#define AP_INTERFACE(STRUCT) \
    virtual ~STRUCT() = default; \
    static_assert(true)
