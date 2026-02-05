#pragma once

#include "importer.hpp"

#include <memory>
#include <vector>

namespace april::asset
{
    class ImporterRegistry
    {
    public:
        auto registerImporter(std::unique_ptr<IImporter> importer) -> void
        {
            m_importers.push_back(std::move(importer));
        }

        [[nodiscard]] auto findImporter(AssetType type) const -> IImporter*
        {
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
        std::vector<std::unique_ptr<IImporter>> m_importers{};
    };
} // namespace april::asset
