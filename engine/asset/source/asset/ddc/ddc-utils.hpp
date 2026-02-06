#pragma once

#include "../dependency.hpp"

#include <core/file/vfs.hpp>
#include <core/tools/hash.hpp>
#include <core/tools/sha1.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
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
        auto file = VFS::open(path);
        if (!file)
        {
            return core::computeStringHash("missing");
        }

        auto hash = core::Sha1{};
        auto buffer = std::array<std::byte, 64 * 1024>{};

        while (true)
        {
            auto const readCount = file->read(buffer);
            if (readCount == 0)
            {
                break;
            }

            hash.update(reinterpret_cast<char const*>(buffer.data()), readCount);
        }

        return hash.getHexDigest();
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
