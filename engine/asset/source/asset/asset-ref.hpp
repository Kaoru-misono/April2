#pragma once

#include <core/tools/uuid.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace april::asset
{
    struct AssetRef
    {
        core::UUID guid{};
        uint32_t subId = 0;
    };

    auto to_json(nlohmann::json& j, AssetRef const& ref) -> void;
    auto from_json(nlohmann::json const& j, AssetRef& ref) -> void;
} // namespace april::asset
