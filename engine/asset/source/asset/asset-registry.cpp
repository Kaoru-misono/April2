#include "asset-registry.hpp"

#include <core/log/logger.hpp>

#include <fstream>
#include <utility>

namespace april::asset
{
    auto AssetRegistry::registerAsset(Asset const& asset, std::string assetPath) -> void
    {
        auto resolvedPath = std::move(assetPath);
        auto record = AssetRecord{};
        record.guid = asset.getHandle();
        record.assetPath = resolvedPath;
        record.type = asset.getType();

        auto lock = std::scoped_lock{m_mutex};
        if (auto it = m_records.find(record.guid); it != m_records.end())
        {
            record = it->second;
            record.guid = asset.getHandle();
            record.assetPath = std::move(resolvedPath);
            record.type = asset.getType();
        }

        updateRecordLocked(std::move(record));
    }

    auto AssetRegistry::updateRecord(AssetRecord record) -> void
    {
        auto lock = std::scoped_lock{m_mutex};
        updateRecordLocked(std::move(record));
    }

    auto AssetRegistry::findRecord(core::UUID const& guid) const -> std::optional<AssetRecord>
    {
        auto lock = std::scoped_lock{m_mutex};
        auto it = m_records.find(guid);
        if (it == m_records.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    auto AssetRegistry::getDependents(core::UUID const& guid) const -> std::vector<core::UUID>
    {
        auto lock = std::scoped_lock{m_mutex};
        auto it = m_dependents.find(guid);
        if (it == m_dependents.end())
        {
            return {};
        }

        auto dependents = std::vector<core::UUID>{};
        dependents.reserve(it->second.size());
        for (auto const& dependent : it->second)
        {
            dependents.push_back(dependent);
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

        auto lock = std::scoped_lock{m_mutex};
        m_records.clear();
        m_dependents.clear();
        for (auto const& entry : json)
        {
            auto record = entry.get<AssetRecord>();
            m_records[record.guid] = std::move(record);
            addDependentsLocked(m_records.at(record.guid));
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

        auto lock = std::scoped_lock{m_mutex};
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
        if (!record.lastSourceHash.empty())
        {
            j["lastSourceHash"] = record.lastSourceHash;
        }
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
            auto const& sourceHash = j.at("lastSourceHash");
            if (sourceHash.is_string())
            {
                record.lastSourceHash = sourceHash.get<std::string>();
            }
            else if (sourceHash.is_object() && !sourceHash.empty())
            {
                record.lastSourceHash = sourceHash.begin().value().get<std::string>();
            }
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

    auto AssetRegistry::updateRecordLocked(AssetRecord record) -> void
    {
        auto const guid = record.guid;
        if (auto it = m_records.find(guid); it != m_records.end())
        {
            removeDependentsLocked(it->second);
            it->second = std::move(record);
            addDependentsLocked(it->second);
            return;
        }

        m_records.emplace(guid, std::move(record));
        addDependentsLocked(m_records.at(guid));
    }

    auto AssetRegistry::addDependentsLocked(AssetRecord const& record) -> void
    {
        for (auto const& dep : record.deps)
        {
            if (dep.kind != DepKind::Strong)
            {
                continue;
            }

            m_dependents[dep.asset.guid].insert(record.guid);
        }
    }

    auto AssetRegistry::removeDependentsLocked(AssetRecord const& record) -> void
    {
        for (auto const& dep : record.deps)
        {
            if (dep.kind != DepKind::Strong)
            {
                continue;
            }

            auto it = m_dependents.find(dep.asset.guid);
            if (it == m_dependents.end())
            {
                continue;
            }

            it->second.erase(record.guid);
            if (it->second.empty())
            {
                m_dependents.erase(it);
            }
        }
    }
} // namespace april::asset
