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
        virtual auto supports(AssetType type) const -> bool = 0;

        virtual auto import(ImportContext const& context) -> ImportResult = 0;
    };
} // namespace april::asset
