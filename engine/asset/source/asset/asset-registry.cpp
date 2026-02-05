#include "asset-registry.hpp"

#include <core/log/logger.hpp>

#include <fstream>
#include <utility>

namespace april::asset
{
    auto AssetRegistry::registerAsset(Asset const& asset, std::string assetPath) -> void
    {
        auto recordOpt = findRecord(asset.getHandle());
        auto record = recordOpt.value_or(AssetRecord{});
        record.guid = asset.getHandle();
        record.assetPath = std::move(assetPath);
        record.type = asset.getType();
        updateRecord(std::move(record));
    }

    auto AssetRegistry::updateRecord(AssetRecord record) -> void
    {
        m_records[record.guid] = std::move(record);
    }

    auto AssetRegistry::findRecord(core::UUID const& guid) const -> std::optional<AssetRecord>
    {
        if (m_records.contains(guid))
        {
            return m_records.at(guid);
        }

        return std::nullopt;
    }

    auto AssetRegistry::getDependents(core::UUID const& guid) const -> std::vector<core::UUID>
    {
        auto dependents = std::vector<core::UUID>{};

        for (auto const& [recordGuid, record] : m_records)
        {
            for (auto const& dep : record.deps)
            {
                if (dep.kind == DepKind::Strong && dep.asset.guid == guid)
                {
                    dependents.push_back(recordGuid);
                    break;
                }
            }
        }

        return dependents;
    }

    auto AssetRegistry::load(std::filesystem::path const& path) -> bool
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        auto file = std::ifstream{path};
        if (!file.is_open())
        {
            AP_ERROR("[AssetRegistry] Failed to open registry: {}", path.string());
            return false;
        }

        auto json = nlohmann::json{};
        file >> json;

        m_records.clear();
        for (auto const& entry : json)
        {
            auto record = entry.get<AssetRecord>();
            m_records[record.guid] = std::move(record);
        }

        return true;
    }

    auto AssetRegistry::save(std::filesystem::path const& path) const -> bool
    {
        auto file = std::ofstream{path};
        if (!file.is_open())
        {
            AP_ERROR("[AssetRegistry] Failed to write registry: {}", path.string());
            return false;
        }

        auto json = nlohmann::json::array();
        for (auto const& [guid, record] : m_records)
        {
            json.push_back(record);
        }

        file << json.dump(4);
        return true;
    }

    auto to_json(nlohmann::json& j, AssetRecord const& record) -> void
    {
        j["guid"] = record.guid.toString();
        j["assetPath"] = record.assetPath;
        j["type"] = record.type;
        j["deps"] = record.deps;
        j["lastSourceHash"] = record.lastSourceHash;
        j["lastFingerprint"] = record.lastFingerprint;
        j["ddcKeys"] = record.ddcKeys;
        j["lastImportFailed"] = record.lastImportFailed;
        j["lastErrorSummary"] = record.lastErrorSummary;
    }

    auto from_json(nlohmann::json const& j, AssetRecord& record) -> void
    {
        if (j.contains("guid"))
        {
            record.guid = core::UUID{j.at("guid").get<std::string>()};
        }
        if (j.contains("assetPath"))
        {
            record.assetPath = j.at("assetPath").get<std::string>();
        }
        if (j.contains("type"))
        {
            record.type = j.at("type").get<AssetType>();
        }
        if (j.contains("deps"))
        {
            record.deps = j.at("deps").get<std::vector<Dependency>>();
        }
        if (j.contains("lastFingerprint"))
        {
            record.lastFingerprint = j.at("lastFingerprint").get<std::unordered_map<std::string, std::string>>();
        }
        if (j.contains("lastSourceHash"))
        {
            record.lastSourceHash = j.at("lastSourceHash").get<std::unordered_map<std::string, std::string>>();
        }
        if (j.contains("ddcKeys"))
        {
            record.ddcKeys = j.at("ddcKeys").get<std::unordered_map<std::string, std::vector<std::string>>>();
        }
        if (j.contains("lastImportFailed"))
        {
            record.lastImportFailed = j.at("lastImportFailed").get<bool>();
        }
        if (j.contains("lastErrorSummary"))
        {
            record.lastErrorSummary = j.at("lastErrorSummary").get<std::string>();
        }
    }
} // namespace april::asset
