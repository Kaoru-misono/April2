// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "types.hpp"
#include "native-handle.hpp"

#include <core/math/type.hpp>
#include <core/foundation/object.hpp>
#include <core/tools/enum.hpp>
#include <slang-rhi.h>

namespace april::graphics
{
    class Device;

    enum class TextureFilteringMode
    {
        Point,
        Linear,
    };
    AP_ENUM_INFO(
        TextureFilteringMode,
        {
            {TextureFilteringMode::Point, "Point"},
            {TextureFilteringMode::Linear, "Linear"},
        }
    );
    AP_ENUM_REGISTER(TextureFilteringMode);

    enum class TextureAddressingMode
    {
        Wrap,      ///< Wrap around
        Mirror,    ///< Wrap around and mirror on every integer junction
        Clamp,     ///< Clamp the normalized coordinates to [0, 1]
        Border,    ///< If out-of-bound, use the sampler's border color
        MirrorOnce ///< Same as Mirror, but mirrors only once around 0
    };
    AP_ENUM_INFO(
        TextureAddressingMode,
        {
            {TextureAddressingMode::Wrap, "Wrap"},
            {TextureAddressingMode::Mirror, "Mirror"},
            {TextureAddressingMode::Clamp, "Clamp"},
            {TextureAddressingMode::Border, "Border"},
            {TextureAddressingMode::MirrorOnce, "MirrorOnce"},
        }
    );
    AP_ENUM_REGISTER(TextureAddressingMode);

    enum class TextureReductionMode
    {
        Standard,
        Comparison,
        Min,
        Max,
    };
    AP_ENUM_INFO(
        TextureReductionMode,
        {
            {TextureReductionMode::Standard, "Standard"},
            {TextureReductionMode::Comparison, "Comparison"},
            {TextureReductionMode::Min, "Min"},
            {TextureReductionMode::Max, "Max"},
        }
    );
    AP_ENUM_REGISTER(TextureReductionMode);

    /**
     * Abstract the API sampler state object
     */
    class Sampler : public core::Object
    {
        APRIL_OBJECT(Sampler)
    public:
        /**
         * Descriptor used to create a new Sampler object
         */
        struct Desc
        {
            TextureFilteringMode magFilter{TextureFilteringMode::Linear};
            TextureFilteringMode minFilter{TextureFilteringMode::Linear};
            TextureFilteringMode mipFilter{TextureFilteringMode::Linear};
            uint32_t maxAnisotropy{1};
            float maxLod{1000};
            float minLod{-1000};
            float lodBias{0};
            ComparisonFunc comparisonFunc{ComparisonFunc::Disabled};
            TextureReductionMode reductionMode{TextureReductionMode::Standard};
            TextureAddressingMode addressModeU{TextureAddressingMode::Wrap};
            TextureAddressingMode addressModeV{TextureAddressingMode::Wrap};
            TextureAddressingMode addressModeW{TextureAddressingMode::Wrap};
            float4 borderColor{0, 0, 0, 0};

            /**
             * Set the filter mode
             * @param[in] min Filter mode in case of minification.
             * @param[in] mag Filter mode in case of magnification.
             * @param[in] mip Mip-level sampling mode
             */
            auto setFilterMode(TextureFilteringMode min, TextureFilteringMode mag, TextureFilteringMode mip) -> Desc&
            {
                magFilter = mag;
                minFilter = min;
                mipFilter = mip;
                return *this;
            }

            /**
             * Set the maximum anisotropic filtering value. If MaxAnisotropy > 1, min/mag/mip filter modes are ignored
             */
            auto setMaxAnisotropy(uint32_t val) -> Desc& { maxAnisotropy = val; return *this; }

            /**
             * Set the lod clamp parameters
             * @param[in] min Minimum LOD that will be used when sampling
             * @param[in] max Maximum LOD that will be used when sampling
             * @param[in] bias Bias to apply to the LOD
             */
            auto setLodParams(float min, float max, float bias) -> Desc& { minLod = min; maxLod = max; lodBias = bias; return *this; }

            /**
             * Set the sampler comparison function.
             */
            auto setComparisonFunc(ComparisonFunc func) -> Desc& { comparisonFunc = func; return *this; }

            /**
             * Set the sampler reduction mode.
             */
            auto setReductionMode(TextureReductionMode mode) -> Desc& { reductionMode = mode; return *this; }

            /**
             * Set the sampler addressing mode
             * @param[in] u Addressing mode for U texcoord channel
             * @param[in] v Addressing mode for V texcoord channel
             * @param[in] w Addressing mode for W texcoord channel
             */
            auto setAddressingMode(TextureAddressingMode u, TextureAddressingMode v, TextureAddressingMode w) -> Desc&
            {
                addressModeU = u;
                addressModeV = v;
                addressModeW = w;
                return *this;
            }

            /**
             * Set the border color. Only applies when the addressing mode is ClampToBorder
             */
            auto setBorderColor(float4 const& color) -> Desc& { borderColor = color; return *this; }

            /**
             * Returns true if sampler descs are identical.
             */
            auto operator==(Desc const& other) const -> bool
            {
                return magFilter == other.magFilter && minFilter == other.minFilter && mipFilter == other.mipFilter &&
                       maxAnisotropy == other.maxAnisotropy && maxLod == other.maxLod && minLod == other.minLod && lodBias == other.lodBias &&
                       comparisonFunc == other.comparisonFunc && reductionMode == other.reductionMode && addressModeU == other.addressModeU &&
                       addressModeV == other.addressModeV && addressModeW == other.addressModeW && borderColor == other.borderColor;
            }

            /**
             * Returns true if sampler descs are not identical.
             */
            auto operator!=(Desc const& other) const -> bool { return !(*this == other); }
        };

        Sampler(core::ref<Device> const& p_device, Desc const& desc);
        ~Sampler();

        /**
         * Get the sampler state.
         */
        auto getGfxSamplerState() const -> rhi::ISampler* { return m_gfxSampler; }

        /**
         * Returns the native API handle:
         * - D3D12: D3D12_CPU_DESCRIPTOR_HANDLE
         * - Vulkan: VkSampler
         */
        auto getNativeHandle() const -> rhi::NativeHandle;

        auto getMagFilter() const -> TextureFilteringMode { return m_desc.magFilter; }
        auto getMinFilter() const -> TextureFilteringMode { return m_desc.minFilter; }
        auto getMipFilter() const -> TextureFilteringMode { return m_desc.mipFilter; }
        auto getMaxAnisotropy() const -> uint32_t { return m_desc.maxAnisotropy; }
        auto getMinLod() const -> float { return m_desc.minLod; }
        auto getMaxLod() const -> float { return m_desc.maxLod; }
        auto getLodBias() const -> float { return m_desc.lodBias; }
        auto getComparisonFunc() const -> ComparisonFunc { return m_desc.comparisonFunc; }
        auto getReductionMode() const -> TextureReductionMode { return m_desc.reductionMode; }
        auto getAddressModeU() const -> TextureAddressingMode { return m_desc.addressModeU; }
        auto getAddressModeV() const -> TextureAddressingMode { return m_desc.addressModeV; }
        auto getAddressModeW() const -> TextureAddressingMode { return m_desc.addressModeW; }
        auto getBorderColor() const -> float4 const& { return m_desc.borderColor; }

        /**
         * Get the descriptor that was used to create the sampler.
         */
        auto getDesc() const -> Desc const& { return m_desc; }

        auto breakStrongReferenceToDevice() -> void;

    private:
        core::BreakableReference<Device> mp_device;
        Desc m_desc{};
        rhi::ComPtr<rhi::ISampler> m_gfxSampler{};
        static auto getApiMaxAnisotropy() -> uint32_t;

        friend class Device;
    };
}
