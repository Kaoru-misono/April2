#pragma once

#include "importer.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace april::asset
{
    class ImporterRegistry
    {
    public:
        auto registerImporter(std::unique_ptr<IImporter> importer) -> void
        {
            auto* rawImporter = importer.get();
            if (rawImporter)
            {
                m_importersById[std::string{rawImporter->id()}] = rawImporter;
            }
            m_importers.push_back(std::move(importer));
            cacheImporter(rawImporter);
        }

        [[nodiscard]] auto findImporter(AssetType type) const -> IImporter*
        {
            auto const index = static_cast<size_t>(type);
            if (index < m_importersByType.size() && m_importersByType[index])
            {
                return m_importersByType[index];
            }
            return nullptr;
        }

        [[nodiscard]] auto findImporterByExtension(std::string_view extension) const -> IImporter*
        {
            for (auto const& importer : m_importers)
            {
                if (importer->supportsExtension(extension))
                {
                    return importer.get();
                }
            }
            return nullptr;
        }

        [[nodiscard]] auto findImporterById(std::string_view importerId) const -> IImporter*
        {
            auto it = m_importersById.find(std::string{importerId});
            if (it != m_importersById.end())
            {
                return it->second;
            }
            return nullptr;
        }

    private:
        static constexpr auto kAssetTypeCount = static_cast<size_t>(AssetType::Material) + 1;

        auto cacheImporter(IImporter* importer) -> void
        {
            if (!importer)
            {
                return;
            }

            for (size_t i = 0; i < kAssetTypeCount; ++i)
            {
                if (m_importersByType[i])
                {
                    continue;
                }

                auto const type = static_cast<AssetType>(i);
                if (importer->primaryType() == type)
                {
                    m_importersByType[i] = importer;
                }
            }
        }

        std::vector<std::unique_ptr<IImporter>> m_importers{};
        std::unordered_map<std::string, IImporter*> m_importersById{};
        std::array<IImporter*, kAssetTypeCount> m_importersByType{};
    };
} // namespace april::asset
