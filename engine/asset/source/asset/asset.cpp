#include "asset.hpp"
#include "asset.hpp"

#include <core/error/assert.hpp>

#include <format>
#include <utility>

namespace april::asset
{
    Asset::Asset(AssetType type)
        : m_handle{UUID()}
        , m_type{type}
    {}

    auto Asset::getHandle() const -> UUID { return m_handle; }
    auto Asset::getType() const -> AssetType { return m_type; }

    auto Asset::getSourcePath() const -> std::string const& { return m_sourcePath; }
    auto Asset::setSourcePath(std::string_view path) -> void { m_sourcePath = path; }

    auto Asset::getAssetPath() const -> std::string const& { return m_assetPath; }
    auto Asset::setAssetPath(std::string_view path) -> void { m_assetPath = path; }

    auto Asset::getImporterChain() const -> std::string const& { return m_importerChain; }
    auto Asset::setImporterChain(std::string importerChain) -> void
    {
        m_importerChain = std::move(importerChain);
    }

    auto Asset::getReferences() const -> std::vector<AssetRef> const& { return m_references; }
    auto Asset::setReferences(std::vector<AssetRef> references) -> void { m_references = std::move(references); }

    auto Asset::serializeJson(nlohmann::json& outJson) -> void
    {
        outJson["guid"] = m_handle.toString();

        outJson["type"] = m_type;

        outJson["source_path"] = m_sourcePath;

        if (!m_importerChain.empty())
        {
            outJson["importer"] = m_importerChain;
        }

        if (!m_references.empty())
        {
            outJson["refs"] = m_references;
        }
    }

    auto Asset::deserializeJson(nlohmann::json const& inJson) -> bool
    {
        if (inJson.contains("guid"))
        {
            m_handle = UUID(inJson["guid"].get<std::string>());
        }

        if (inJson.contains("type"))
        {
            AssetType jsonType = inJson["type"].get<AssetType>();
            AP_ASSERT(jsonType == m_type, "Asset type mismatch in JSON!");
        }

        if (inJson.contains("source_path"))
        {
            m_sourcePath = inJson["source_path"].get<std::string>();
        }

        if (inJson.contains("importer"))
        {
            auto const& importer = inJson["importer"];
            if (importer.is_string())
            {
                m_importerChain = importer.get<std::string>();
            }
            else if (importer.is_object())
            {
                if (importer.contains("id") && importer.contains("version"))
                {
                    auto importerId = importer["id"].get<std::string>();
                    auto importerVersion = importer["version"].get<int>();
                    m_importerChain = std::format("{}@v{}", importerId, importerVersion);
                }
            }
        }

        if (inJson.contains("refs"))
        {
            m_references = inJson["refs"].get<std::vector<AssetRef>>();
        }

        return true;
    }
}
