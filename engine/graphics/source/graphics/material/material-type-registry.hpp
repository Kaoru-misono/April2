#pragma once

#include <string>
#include <unordered_map>

namespace april::graphics
{
    class MaterialTypeRegistry
    {
    public:
        using MaterialTypeId = unsigned int;
        static constexpr MaterialTypeId kInvalidMaterialTypeId = 0;

        MaterialTypeRegistry();

        auto registerBuiltIn(std::string const& typeName, MaterialTypeId typeId) -> MaterialTypeId;
        auto registerExtension(std::string const& typeName) -> MaterialTypeId;

        auto resolveTypeId(std::string const& typeName) const -> MaterialTypeId;
        auto resolveTypeName(MaterialTypeId typeId) const -> std::string;

    private:
        std::unordered_map<std::string, MaterialTypeId> m_typeIdsByName{};
        std::unordered_map<MaterialTypeId, std::string> m_typeNamesById{};

        static auto hashTypeName(std::string const& typeName) -> MaterialTypeId;
    };
}
