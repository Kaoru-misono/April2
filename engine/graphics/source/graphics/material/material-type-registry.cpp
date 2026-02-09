#include "material-type-registry.hpp"

namespace april::graphics
{
    MaterialTypeRegistry::MaterialTypeRegistry()
    {
        registerBuiltIn("Unknown", kInvalidMaterialTypeId);
    }

    auto MaterialTypeRegistry::registerBuiltIn(std::string const& typeName, MaterialTypeId typeId) -> MaterialTypeId
    {
        if (typeName.empty())
        {
            return kInvalidMaterialTypeId;
        }

        m_typeIdsByName[typeName] = typeId;
        m_typeNamesById[typeId] = typeName;
        return typeId;
    }

    auto MaterialTypeRegistry::registerExtension(std::string const& typeName) -> MaterialTypeId
    {
        if (typeName.empty())
        {
            return kInvalidMaterialTypeId;
        }

        if (auto const found = m_typeIdsByName.find(typeName); found != m_typeIdsByName.end())
        {
            return found->second;
        }

        auto typeId = hashTypeName(typeName);
        while (typeId == kInvalidMaterialTypeId || m_typeNamesById.contains(typeId))
        {
            ++typeId;
        }

        m_typeIdsByName[typeName] = typeId;
        m_typeNamesById[typeId] = typeName;
        return typeId;
    }

    auto MaterialTypeRegistry::resolveTypeId(std::string const& typeName) const -> MaterialTypeId
    {
        if (auto const found = m_typeIdsByName.find(typeName); found != m_typeIdsByName.end())
        {
            return found->second;
        }

        return kInvalidMaterialTypeId;
    }

    auto MaterialTypeRegistry::resolveTypeName(MaterialTypeId typeId) const -> std::string
    {
        if (auto const found = m_typeNamesById.find(typeId); found != m_typeNamesById.end())
        {
            return found->second;
        }

        return "Unknown";
    }

    auto MaterialTypeRegistry::hashTypeName(std::string const& typeName) -> MaterialTypeId
    {
        constexpr unsigned int fnvOffset = 2166136261u;
        constexpr unsigned int fnvPrime = 16777619u;

        auto hash = fnvOffset;
        for (char const c : typeName)
        {
            hash ^= static_cast<unsigned char>(c);
            hash *= fnvPrime;
        }

        if (hash == kInvalidMaterialTypeId)
        {
            return 1;
        }
        return hash;
    }
}
