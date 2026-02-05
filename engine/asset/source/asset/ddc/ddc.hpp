#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace april::asset
{
    struct DdcValue
    {
        std::vector<std::byte> bytes{};
        std::string contentHash{};
    };

    class IDdc
    {
    public:
        virtual ~IDdc() = default;

        virtual auto get(std::string const& key, DdcValue& outValue) -> bool = 0;
        virtual auto put(std::string const& key, DdcValue const& value) -> void = 0;
        virtual auto exists(std::string const& key) -> bool = 0;
    };
} // namespace april::asset
