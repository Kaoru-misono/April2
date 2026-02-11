#pragma once

#include "rhi/format.hpp"
#include "rhi/texture.hpp"

#include <core/tools/enum.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace april::graphics
{
    class Buffer;
    class CommandContext;

    class TextureAnalyzer
    {
    public:
        struct Result
        {
            uint32_t mask{0};
            uint32_t reserved[3]{};

            float4 value{0.0f};
            float4 minValue{0.0f};
            float4 maxValue{0.0f};

            enum class RangeFlags : uint32_t
            {
                Pos = 0x1,
                Neg = 0x2,
                Inf = 0x4,
                NaN = 0x8,
            };

            auto isConstant(uint32_t channelMask) const -> bool { return (mask & channelMask) == 0; }
            auto isConstant(TextureChannelFlags channelMask) const -> bool { return isConstant(static_cast<uint32_t>(channelMask)); }

            auto getRange(TextureChannelFlags channelMask) const -> uint32_t
            {
                auto range = uint32_t{0};
                for (auto i = 0; i < 4; ++i)
                {
                    if (static_cast<uint32_t>(channelMask) & (1u << i))
                    {
                        range |= mask >> (4 + 4 * i);
                    }
                }
                return range & 0xf;
            }
        };

        static auto getResultSize() -> size_t { return sizeof(Result); }

        auto analyze(core::ref<Texture> const& p_texture) const -> Result;
        auto analyze(std::vector<core::ref<Texture>> const& textures) const -> std::vector<Result>;
        auto analyze(CommandContext* p_context, std::vector<core::ref<Texture>> const& textures, core::ref<Buffer> const& p_results) const -> void;
    };

    AP_ENUM_CLASS_OPERATORS(TextureAnalyzer::Result::RangeFlags);
}
