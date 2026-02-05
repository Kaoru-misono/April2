#pragma once

#include <string>

namespace april::asset
{
    struct TargetProfile
    {
        std::string platform{"Win64"};
        std::string gpuFormat{"BC7"};
        std::string quality{"Debug"};

        [[nodiscard]] auto toId() const -> std::string
        {
            return platform + "|" + gpuFormat + "|" + quality;
        }
    };
} // namespace april::asset
