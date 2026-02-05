#pragma once

#include "../dependency.hpp"

#include <core/tools/hash.hpp>

#include <fstream>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace april::asset
{
    inline auto hashJson(nlohmann::json const& json) -> std::string
    {
        return core::computeStringHash(json.dump());
    }

    inline auto hashFileContents(std::string const& path) -> std::string
    {
        auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
        if (!file.is_open())
        {
            return core::computeStringHash("missing");
        }

        auto size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        auto data = std::string{};
        data.resize(size);
        file.read(data.data(), static_cast<std::streamsize>(size));

        return core::computeStringHash(data);
    }

    inline auto hashDependencies(std::vector<Dependency> const& deps) -> std::string
    {
        auto strongDeps = std::vector<Dependency>{};
        strongDeps.reserve(deps.size());

        for (auto const& dep : deps)
        {
            if (dep.kind == DepKind::Strong)
            {
                strongDeps.push_back(dep);
            }
        }

        std::sort(strongDeps.begin(), strongDeps.end(),
            [](Dependency const& lhs, Dependency const& rhs)
            {
                auto lhsGuid = lhs.asset.guid.toString();
                auto rhsGuid = rhs.asset.guid.toString();
                if (lhsGuid != rhsGuid)
                {
                    return lhsGuid < rhsGuid;
                }
                if (lhs.asset.subId != rhs.asset.subId)
                {
                    return lhs.asset.subId < rhs.asset.subId;
                }
                return lhs.kind < rhs.kind;
            });

        auto combined = std::string{};
        for (auto const& dep : strongDeps)
        {
            combined += dep.asset.guid.toString();
            combined += ":";
            combined += std::to_string(dep.asset.subId);
            combined += "|";
        }

        return core::computeStringHash(combined);
    }
} // namespace april::asset
