#pragma once

#include "../dependency.hpp"
#include "../target-profile.hpp"
#include "../ddc/ddc.hpp"
#include "../asset.hpp"

#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace april::asset
{
    struct DepRecorder
    {
        std::vector<Dependency> deps{};

        auto addStrong(AssetRef const& asset) -> void { deps.push_back(Dependency{DepKind::Strong, asset}); }
        auto addWeak(AssetRef const& asset) -> void { deps.push_back(Dependency{DepKind::Weak, asset}); }
    };

    struct ImportSourceContext
    {
        std::filesystem::path sourcePath{};
        std::string importerChain{};

        // Check if asset exists by source path (for deduplication)
        std::function<std::shared_ptr<Asset>(std::filesystem::path const&, AssetType)> findAssetBySource{};

        // Import a sub-asset using another importer
        std::function<std::shared_ptr<Asset>(std::filesystem::path const&)> importSubAsset{};
    };

    struct ImportCookContext
    {
        Asset const& asset;
        std::string assetPath{};
        std::string sourcePath{};
        TargetProfile target{};
        IDdc& ddc;
        DepRecorder& deps;
        bool forceReimport = false;
    };

    struct ImportSourceResult
    {
        std::shared_ptr<Asset> primaryAsset{};
        std::vector<std::shared_ptr<Asset>> assets{};
        std::vector<std::string> warnings{};
        std::vector<std::string> errors{};
    };

    struct ImportCookResult
    {
        std::vector<std::string> producedKeys{};
        std::vector<std::string> warnings{};
        std::vector<std::string> errors{};
    };

    inline auto formatImporterTag(std::string_view importerId, int importerVersion) -> std::string
    {
        return std::format("{}@v{}", importerId, importerVersion);
    }

    inline auto appendImporterChain(
        std::string const& chain,
        std::string_view importerId,
        int importerVersion
    ) -> std::string
    {
        auto tag = formatImporterTag(importerId, importerVersion);
        if (chain.empty())
        {
            return tag;
        }

        if (chain.ends_with(tag))
        {
            return chain;
        }

        return std::format("{}|{}", chain, tag);
    }
} // namespace april::asset
