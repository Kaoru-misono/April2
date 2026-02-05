#pragma once

#include "../dependency.hpp"
#include "../target-profile.hpp"
#include "../ddc/ddc.hpp"
#include "../asset.hpp"

#include <filesystem>
#include <functional>
#include <memory>
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

        // Check if asset exists by source path (for deduplication)
        std::function<std::shared_ptr<Asset>(std::filesystem::path const&, AssetType)> findAssetBySource{};

        // Register a produced sub-asset
        std::function<void(std::shared_ptr<Asset> const&)> registerSubAsset{};

        // Import a sub-asset (for texture imports)
        std::function<std::shared_ptr<Asset>(std::filesystem::path const&)> importSubAsset{};
    };

    struct ProducedAsset
    {
        AssetType type{AssetType::None};
        core::UUID guid{};
        std::string assetPath{};
    };

    struct ImportResult
    {
        std::vector<ProducedAsset> producedAssets{};
        std::vector<std::string> producedKeys{};
        std::vector<std::string> warnings{};
        std::vector<std::string> errors{};
    };
} // namespace april::asset
