#include "material-type-registry.hpp"
#include "../generated/material/material-data.generated.hpp"

#include <core/log/logger.hpp>
#include <mutex>

namespace april::graphics
{
    MaterialTypeRegistry::MaterialTypeRegistry()
    {
        registerBuiltIn("Unknown", kInvalidMaterialTypeId);
        m_nextExtensionTypeId = 1;
    }

    auto MaterialTypeRegistry::registerBuiltIn(std::string const& typeName, MaterialTypeId typeId) -> MaterialTypeId
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);
        if (typeName.empty())
        {
            return kInvalidMaterialTypeId;
        }

        m_typeIdsByName[typeName] = typeId;
        m_typeNamesById[typeId] = typeName;
        if (typeId >= m_nextExtensionTypeId)
        {
            m_nextExtensionTypeId = typeId + 1;
        }
        return typeId;
    }

    auto MaterialTypeRegistry::registerExtension(std::string const& typeName) -> MaterialTypeId
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);
        if (typeName.empty())
        {
            return kInvalidMaterialTypeId;
        }

        if (auto const found = m_typeIdsByName.find(typeName); found != m_typeIdsByName.end())
        {
            return found->second;
        }

        auto constexpr kMaxMaterialType = (1u << generated::MaterialHeader::kMaterialTypeBits);
        if (m_nextExtensionTypeId >= kMaxMaterialType)
        {
            AP_ERROR("MaterialTypeRegistry exhausted material type id budget ({}).", kMaxMaterialType);
            return kInvalidMaterialTypeId;
        }

        auto typeId = m_nextExtensionTypeId++;

        m_typeIdsByName[typeName] = typeId;
        m_typeNamesById[typeId] = typeName;
        return typeId;
    }

    auto MaterialTypeRegistry::resolveTypeId(std::string const& typeName) const -> MaterialTypeId
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);
        if (auto const found = m_typeIdsByName.find(typeName); found != m_typeIdsByName.end())
        {
            return found->second;
        }

        return kInvalidMaterialTypeId;
    }

    auto MaterialTypeRegistry::resolveTypeName(MaterialTypeId typeId) const -> std::string
    {
        auto lock = std::lock_guard<std::mutex>(m_mutex);
        if (auto const found = m_typeNamesById.find(typeId); found != m_typeNamesById.end())
        {
            return found->second;
        }

        return "Unknown";
    }
}
