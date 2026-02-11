// IMaterial interface - Abstract base class for materials.

#pragma once

#include "generated/material/material-types.generated.hpp"
#include "generated/material/material-data.generated.hpp"
#include "../program/define-list.hpp"
#include "program/program.hpp"
#include "rhi/format.hpp"

#include <core/foundation/object.hpp>

#include <functional>

namespace april::graphics
{
    class MaterialSystem;
    class ShaderVariable;
    class Texture;
    class Sampler;

    /**
     * Material update flags (Falcor Material::UpdateFlags).
     * Indicates what was updated in the material since last update().
     */
    enum class MaterialUpdateFlags : uint32_t
    {
        None                = 0x0,   // Nothing updated.
        CodeChanged         = 0x1,   // Material shader code changed.
        DataChanged         = 0x2,   // Material data (parameters) changed.
        ResourcesChanged    = 0x4,   // Material resources (textures, buffers, samplers) changed.
        DisplacementChanged = 0x8,   // Displacement mapping parameters changed.
        EmissiveChanged     = 0x10,  // Material emissive properties changed.
    };

    inline auto operator|(MaterialUpdateFlags a, MaterialUpdateFlags b) -> MaterialUpdateFlags
    {
        return static_cast<MaterialUpdateFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline auto operator&(MaterialUpdateFlags a, MaterialUpdateFlags b) -> MaterialUpdateFlags
    {
        return static_cast<MaterialUpdateFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline auto operator|=(MaterialUpdateFlags& a, MaterialUpdateFlags b) -> MaterialUpdateFlags&
    {
        a = a | b;
        return a;
    }

    inline auto hasFlag(MaterialUpdateFlags flags, MaterialUpdateFlags flag) -> bool
    {
        return (flags & flag) != MaterialUpdateFlags::None;
    }

    /**
     * Abstract base class for materials.
     * Materials manage GPU data and textures for rendering.
     */
    class IMaterial : public core::Object
    {
        APRIL_OBJECT(IMaterial)
    public:
        enum class TextureSlot
        {
            BaseColor,
            Specular,
            Emissive,
            Normal,
            Transmission,
            Displacement,
            Count,
        };

        struct TextureSlotInfo
        {
            std::string name;
            TextureChannelFlags mask{TextureChannelFlags::None};
            bool srgb{false};

            auto isEnabled() const -> bool { return mask != TextureChannelFlags::None; }
        };

        using UpdateCallback = std::function<void(MaterialUpdateFlags)>;

        virtual ~IMaterial() = default;

        /**
         * Update material. Prepares material for rendering.
         * @param pOwner Material system that owns this material.
         * @return Update flags since previous update() call.
         */
        virtual auto update(MaterialSystem* pOwner) -> MaterialUpdateFlags = 0;

        /**
         * Get material data blob for GPU upload.
         */
        virtual auto getDataBlob() const -> generated::MaterialDataBlob = 0;

        /**
         * Get the material type.
         */
        virtual auto getType() const -> generated::MaterialType = 0;

        /**
         * Get the material type name as a string.
         */
        virtual auto getTypeName() const -> std::string = 0;

        /**
         * Get type conformances for shader compilation.
         * These specify which implementations to use for interfaces (e.g., IDiffuseBRDF).
         */
        virtual auto getTypeConformances() const -> TypeConformanceList = 0;

        /**
         * Get shader modules required by this material type.
         */
        virtual auto getShaderModules() const -> ProgramDesc::ShaderModuleList = 0;

        /**
         * Get shader defines required by this material type.
         */
        virtual auto getDefines() const -> DefineList { return {}; }

        /**
         * Bind textures to a shader variable.
         * @param var Shader variable representing the material's texture bindings.
         */
        virtual auto bindTextures(ShaderVariable& var) const -> void = 0;

        /**
         * Check if the material has any textures bound.
         */
        virtual auto hasTextures() const -> bool = 0;

        virtual auto getTextureSlotInfo(TextureSlot slot) const -> TextureSlotInfo const&;
        virtual auto hasTextureSlot(TextureSlot slot) const -> bool;
        virtual auto setTexture(TextureSlot slot, core::ref<Texture> texture) -> bool;
        virtual auto getTexture(TextureSlot slot) const -> core::ref<Texture>;

        /**
         * Get the material flags.
         */
        virtual auto getFlags() const -> uint32_t = 0;

        /**
         * Set whether the material is double-sided.
         */
        virtual auto setDoubleSided(bool doubleSided) -> void = 0;

        /**
         * Get whether the material is double-sided.
         */
        virtual auto isDoubleSided() const -> bool = 0;

        virtual auto getMaxTextureCount() const -> size_t { return 7; }
        virtual auto getMaxBufferCount() const -> size_t { return 0; }
        virtual auto getMaxTexture3DCount() const -> size_t { return 0; }

        /**
         * Returns true for dynamic materials requiring per-frame update.
         */
        virtual auto isDynamic() const -> bool { return false; }

        /**
         * Size of IMaterialInstance implementation in bytes.
         */
        virtual auto getMaterialInstanceByteSize() const -> size_t { return 128; }

        auto registerUpdateCallback(UpdateCallback const& callback) -> void { m_updateCallback = callback; }

        auto setDefaultTextureSampler(core::ref<Sampler> sampler) -> void
        {
            m_defaultTextureSampler = std::move(sampler);
            markUpdates(MaterialUpdateFlags::ResourcesChanged);
        }

        auto getDefaultTextureSampler() const -> core::ref<Sampler> const& { return m_defaultTextureSampler; }

        // ---- Lifecycle and update tracking ----

        /**
         * Check if the material has pending updates.
         */
        auto getUpdateFlags() const -> MaterialUpdateFlags { return m_updateFlags; }

    protected:
        auto markUpdates(MaterialUpdateFlags flags) -> void
        {
            if (flags == MaterialUpdateFlags::None)
            {
                return;
            }

            m_updateFlags |= flags;
            if (m_updateCallback)
            {
                m_updateCallback(flags);
            }
        }

        auto consumeUpdates() -> MaterialUpdateFlags
        {
            auto const flags = m_updateFlags;
            m_updateFlags = MaterialUpdateFlags::None;
            return flags;
        }

        static auto getDisabledTextureSlotInfo() -> TextureSlotInfo const&
        {
            static TextureSlotInfo const kDisabled{};
            return kDisabled;
        }

        // Initial state: all updates pending (Falcor uses DataChanged | ResourcesChanged).
        MaterialUpdateFlags m_updateFlags{MaterialUpdateFlags::DataChanged | MaterialUpdateFlags::ResourcesChanged};
        UpdateCallback m_updateCallback{};
        core::ref<Sampler> m_defaultTextureSampler{};
    };

    inline auto IMaterial::getTextureSlotInfo(TextureSlot slot) const -> TextureSlotInfo const&
    {
        (void)slot;
        return getDisabledTextureSlotInfo();
    }

    inline auto IMaterial::hasTextureSlot(TextureSlot slot) const -> bool
    {
        return getTextureSlotInfo(slot).isEnabled();
    }

    inline auto IMaterial::setTexture(TextureSlot slot, core::ref<Texture> texture) -> bool
    {
        (void)slot;
        (void)texture;
        return false;
    }

    inline auto IMaterial::getTexture(TextureSlot slot) const -> core::ref<Texture>
    {
        (void)slot;
        return nullptr;
    }

} // namespace april::graphics
