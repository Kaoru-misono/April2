#pragma once

#include "import-types.hpp"

#include <string_view>

namespace april::asset
{
    class IImporter
    {
    public:
        virtual ~IImporter() = default;

        virtual auto id() const -> std::string_view = 0;
        virtual auto version() const -> int = 0;
        virtual auto supportsExtension(std::string_view extension) const -> bool = 0;
        virtual auto primaryType() const -> AssetType = 0;

        virtual auto import(ImportSourceContext const& context) -> ImportSourceResult = 0;
        virtual auto cook(ImportCookContext const& context) -> ImportCookResult = 0;
    };
} // namespace april::asset
