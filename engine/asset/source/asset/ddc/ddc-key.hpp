#pragma once

#include "../target-profile.hpp"

#include <format>
#include <string>

namespace april::asset
{
    struct FingerprintInput
    {
        std::string typePrefix{};
        std::string guid{};
        std::string importerId{};
        int importerVersion = 0;
        std::string toolchainHash{};
        std::string sourceHash{};
        std::string settingsHash{};
        std::string depsHash{};
        TargetProfile target{};
    };

    inline auto buildDdcKey(FingerprintInput const& input) -> std::string
    {
        auto targetId = input.target.toId();
        return std::format(
            "{}|{}|imp={}@v{}|tgt={}|S={}|C={}|D={}|T={}",
            input.typePrefix,
            input.guid,
            input.importerId,
            input.importerVersion,
            targetId,
            input.settingsHash,
            input.sourceHash,
            input.depsHash,
            input.toolchainHash
        );
    }
} // namespace april::asset
