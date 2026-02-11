#pragma once

#include "generated/material/material-data.generated.hpp"
#include "generated/material/material-types.generated.hpp"
#include "../program/define-list.hpp"
#include "program/program.hpp"
#include "rhi/format.hpp"

#include <core/foundation/object.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>

namespace april::graphics
{
    class Device;
    class MaterialSystem;
    class Texture;
    class Sampler;

    struct MaterialParamLayout
    {
    };

    struct SerializedMaterialParams
    {
    };

    class Material : public core::Object
    {
        APRIL_OBJECT(Material)
    public:
        enum class UpdateFlags : uint32_t
        {
            None                = 0x0,
            CodeChanged         = 0x1,
            DataChanged         = 0x2,
            ResourcesChanged    = 0x4,
            DisplacementChanged = 0x8,
            EmissiveChanged     = 0x10,
        };

        enum class TextureSlot
        {
            BaseColor,
            Specular,
            Emissive,
            Normal,
            Transmission,
            Displacement,
            Index,
            Count,
        };

        struct TextureSlotInfo
        {
            std::string name;
            TextureChannelFlags mask{TextureChannelFlags::None};
            bool srgb{false};

            auto isEnabled() const -> bool { return mask != TextureChannelFlags::None; }
            auto operator==(TextureSlotInfo const& rhs) const -> bool { return name == rhs.name && mask == rhs.mask && srgb == rhs.srgb; }
            auto operator!=(TextureSlotInfo const& rhs) const -> bool { return !(*this == rhs); }
        };

        struct TextureSlotData
        {
            core::ref<Texture> pTexture;

            auto hasData() const -> bool { return pTexture != nullptr; }
            auto operator==(TextureSlotData const& rhs) const -> bool { return pTexture == rhs.pTexture; }
            auto operator!=(TextureSlotData const& rhs) const -> bool { return !(*this == rhs); }
        };

        struct TextureOptimizationStats
        {
            std::array<size_t, static_cast<size_t>(TextureSlot::Count)> texturesRemoved{};
            size_t disabledAlpha{0};
            size_t constantBaseColor{0};
            size_t constantNormalMaps{0};
        };

        virtual ~Material() = default;

        virtual auto update(MaterialSystem* pOwner) -> UpdateFlags = 0;

        virtual auto setName(std::string const& name) -> void;
        virtual auto getName() const -> std::string const&;

        virtual auto getType() const -> generated::MaterialType;
        virtual auto isOpaque() const -> bool;
        virtual auto isDisplaced() const -> bool;
        virtual auto isEmissive() const -> bool;
        virtual auto isDynamic() const -> bool;

        virtual auto isEqual(core::ref<Material> const& pOther) const -> bool = 0;

        virtual auto setDoubleSided(bool doubleSided) -> void;
        virtual auto isDoubleSided() const -> bool;
        virtual auto setThinSurface(bool thinSurface) -> void;
        virtual auto isThinSurface() const -> bool;
        virtual auto setAlphaMode(generated::AlphaMode alphaMode) -> void;
        virtual auto getAlphaMode() const -> generated::AlphaMode;
        virtual auto setAlphaThreshold(float alphaThreshold) -> void;
        virtual auto getAlphaThreshold() const -> float;
        virtual auto getAlphaTextureHandle() const -> generated::TextureHandle;
        virtual auto setNestedPriority(uint32_t priority) -> void;
        virtual auto getNestedPriority() const -> uint32_t;
        virtual auto setIndexOfRefraction(float ior) -> void;
        virtual auto getIndexOfRefraction() const -> float;
        virtual auto getTextureSlotInfo(TextureSlot slot) const -> TextureSlotInfo const&;
        virtual auto hasTextureSlot(TextureSlot slot) const -> bool;
        virtual auto setTexture(TextureSlot slot, core::ref<Texture> const& pTexture) -> bool;
        virtual auto loadTexture(TextureSlot slot, std::filesystem::path const& path, bool useSrgb = true) -> bool;
        virtual auto clearTexture(TextureSlot slot) -> void;
        virtual auto getTexture(TextureSlot slot) const -> core::ref<Texture>;
        virtual auto optimizeTexture(TextureSlot slot, TextureOptimizationStats& stats) -> void;
        virtual auto setDefaultTextureSampler(core::ref<Sampler> const& pSampler) -> void;
        virtual auto getDefaultTextureSampler() const -> core::ref<Sampler>;
        virtual auto getHeader() const -> generated::MaterialHeader const&;

        virtual auto getDataBlob() const -> generated::MaterialDataBlob = 0;
        virtual auto getShaderModules() const -> ProgramDesc::ShaderModuleList = 0;
        virtual auto getTypeConformances() const -> TypeConformanceList = 0;
        virtual auto getDefines() const -> DefineList;

        virtual auto getMaxBufferCount() const -> size_t;
        virtual auto getMaxTextureCount() const -> size_t;
        virtual auto getMaxTexture3DCount() const -> size_t;
        virtual auto getMaterialInstanceByteSize() const -> size_t;

        virtual auto getParamLayout() const -> MaterialParamLayout const&;
        virtual auto serializeParams() const -> SerializedMaterialParams;
        virtual auto deserializeParams(SerializedMaterialParams const& params) -> void;

    protected:
        Material(core::ref<Device> pDevice, std::string const& name, generated::MaterialType type);

        using UpdateCallback = std::function<void(UpdateFlags)>;

        auto registerUpdateCallback(UpdateCallback const& updateCallback) -> void;
        auto markUpdates(UpdateFlags updates) -> void;

        auto hasTextureSlotData(TextureSlot slot) const -> bool;
        auto updateTextureHandle(MaterialSystem* pOwner, core::ref<Texture> const& pTexture, generated::TextureHandle& handle) -> void;
        auto updateTextureHandle(MaterialSystem* pOwner, TextureSlot slot, generated::TextureHandle& handle) -> void;
        auto updateDefaultTextureSamplerID(MaterialSystem* pOwner, core::ref<Sampler> const& pSampler) -> void;
        auto isBaseEqual(Material const& other) const -> bool;
        static auto detectNormalMapType(core::ref<Texture> const& pNormalMap) -> generated::NormalMapType;

        template<typename T>
        auto prepareDataBlob(T const& data) const -> generated::MaterialDataBlob
        {
            auto blob = generated::MaterialDataBlob{};
            blob.header = m_header;
            static_assert(sizeof(m_header) + sizeof(data) <= sizeof(blob));
            static_assert(offsetof(generated::MaterialDataBlob, payload) == sizeof(generated::MaterialHeader));
            std::memcpy(&blob.payload, &data, sizeof(data));
            return blob;
        }

        core::ref<Device> m_device;

        std::string m_name;
        generated::MaterialHeader m_header{};

        std::array<TextureSlotInfo, static_cast<size_t>(TextureSlot::Count)> m_textureSlotInfo{};
        std::array<TextureSlotData, static_cast<size_t>(TextureSlot::Count)> m_textureSlotData{};

        mutable UpdateFlags m_updates{static_cast<UpdateFlags>(static_cast<uint32_t>(UpdateFlags::DataChanged) | static_cast<uint32_t>(UpdateFlags::ResourcesChanged))};
        UpdateCallback m_updateCallback{};

        friend class MaterialSystem;
    };

    using IMaterial = Material;
    using MaterialUpdateFlags = Material::UpdateFlags;

    AP_ENUM_CLASS_OPERATORS(Material::UpdateFlags);

}
