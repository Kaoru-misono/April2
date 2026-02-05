#pragma once

#include "../dependency.hpp"
#include "../target-profile.hpp"
#include "../ddc/ddc.hpp"
#include "../asset.hpp"

#include <string>
#include <vector>

namespace april::asset
{
    struct DepRecorder
    {
        std::vector<Dependency> deps{};

        auto addStrong(AssetRef const& asset) -> void { deps.push_back(Dependency{DepKind::Strong, asset}); }
        auto addWeak(AssetRef const& asset) -> void { deps.push_back(Dependency{DepKind::Weak, asset}); }
    };

    struct ImportContext
    {
        Asset const& asset;
        std::string assetPath{};
        std::string sourcePath{};
        TargetProfile target{};
        IDdc& ddc;
        DepRecorder& deps;
        bool forceReimport = false;
    };

    struct ImportResult
    {
        std::vector<std::string> producedKeys{};
        std::vector<std::string> warnings{};
        std::vector<std::string> errors{};
    };
} // namespace april::asset
