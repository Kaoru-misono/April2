#pragma once

#include "asset-ref.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>

namespace april::asset
{
    enum class DepKind : uint8_t
    {
        Strong,
        Weak
    };

    struct Dependency
    {
        DepKind kind{DepKind::Strong};
        AssetRef asset{};
    };

    auto to_json(nlohmann::json& j, Dependency const& dep) -> void;
    auto from_json(nlohmann::json const& j, Dependency& dep) -> void;
} // namespace april::asset
