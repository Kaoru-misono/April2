#pragma once

#include "importer.hpp"

#include <array>
#include <memory>
#include <vector>

namespace april::asset
{
    class ImporterRegistry
    {
    public:
        auto registerImporter(std::unique_ptr<IImporter> importer) -> void
        {
            auto* rawImporter = importer.get();
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

            for (auto const& importer : m_importers)
            {
                if (importer->supports(type))
                {
                    return importer.get();
                }
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
                if (importer->supports(type))
                {
                    m_importersByType[i] = importer;
                }
            }
        }

        std::vector<std::unique_ptr<IImporter>> m_importers{};
        std::array<IImporter*, kAssetTypeCount> m_importersByType{};
    };
} // namespace april::asset
