#pragma once

#include "dependency.hpp"
#include "target-profile.hpp"
#include "asset.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace april::asset
{
    struct AssetRecord
    {
        core::UUID guid{};
        std::string assetPath{};
        AssetType type{AssetType::None};

        std::vector<Dependency> deps{};

        std::unordered_map<std::string, std::string> lastSourceHash{};
        std::unordered_map<std::string, std::string> lastFingerprint{};
        std::unordered_map<std::string, std::vector<std::string>> ddcKeys{};

        bool lastImportFailed = false;
        std::string lastErrorSummary{};
    };

    class AssetRegistry
    {
    public:
        auto registerAsset(Asset const& asset, std::string assetPath) -> void;
        auto updateRecord(AssetRecord record) -> void;
        [[nodiscard]] auto findRecord(core::UUID const& guid) const -> std::optional<AssetRecord>;
        [[nodiscard]] auto getDependents(core::UUID const& guid) const -> std::vector<core::UUID>;

        auto load(std::filesystem::path const& path) -> bool;
        auto save(std::filesystem::path const& path) const -> bool;

    private:
        auto updateRecordLocked(AssetRecord record) -> void;
        auto addDependentsLocked(AssetRecord const& record) -> void;
        auto removeDependentsLocked(AssetRecord const& record) -> void;

        mutable std::mutex m_mutex{};
        std::unordered_map<core::UUID, AssetRecord> m_records{};
        std::unordered_map<core::UUID, std::unordered_set<core::UUID>> m_dependents{};
    };

    auto to_json(nlohmann::json& j, AssetRecord const& record) -> void;
    auto from_json(nlohmann::json const& j, AssetRecord& record) -> void;
} // namespace april::asset
