// IMaterial interface - Abstract base class for materials.

#pragma once

#include "generated/material/material-types.generated.hpp"
#include "generated/material/material-data.generated.hpp"
#include "program/program.hpp"

#include <core/foundation/object.hpp>
#include <core/math/json.hpp>

namespace april::graphics
{
    class ShaderVariable;
    class Texture;

    /**
     * Material update flags for selective GPU upload.
     */
    enum class MaterialUpdateFlags : uint32_t
    {
        None = 0,
        DataChanged = 1 << 0,       // Material parameters changed
        TexturesChanged = 1 << 1,   // Texture bindings changed
        All = DataChanged | TexturesChanged
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

    /**
     * Abstract base class for materials.
     * Materials manage GPU data and textures for rendering.
     */
    class IMaterial : public core::Object
    {
        APRIL_OBJECT(IMaterial)
    public:
        virtual ~IMaterial() = default;

        /**
         * Get the material type.
         */
        virtual auto getType() const -> generated::MaterialType = 0;

        /**
         * Get the material type name as a string.
         */
        virtual auto getTypeName() const -> std::string = 0;

        /**
         * Write material data to a GPU-compatible struct.
         * @param data Output struct to populate.
         */
        virtual auto writeData(generated::StandardMaterialData& data) const -> void = 0;

        /**
         * Get type conformances for shader compilation.
         * These specify which implementations to use for interfaces (e.g., IDiffuseBRDF).
         */
        virtual auto getTypeConformances() const -> TypeConformanceList = 0;

        /**
         * Bind textures to a shader variable.
         * @param var Shader variable representing the material's texture bindings.
         */
        virtual auto bindTextures(ShaderVariable& var) const -> void = 0;

        /**
         * Check if the material has any textures bound.
         */
        virtual auto hasTextures() const -> bool = 0;

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

        // ---- Lifecycle and update tracking ----

        /**
         * Check if the material has pending updates.
         */
        auto isDirty() const -> bool { return m_updateFlags != MaterialUpdateFlags::None; }

        /**
         * Get pending update flags.
         */
        auto getUpdateFlags() const -> MaterialUpdateFlags { return m_updateFlags; }

        /**
         * Mark the material as needing an update.
         */
        auto markDirty(MaterialUpdateFlags flags = MaterialUpdateFlags::All) -> void { m_updateFlags |= flags; }

        /**
         * Clear update flags after GPU upload.
         */
        auto clearDirty() -> void { m_updateFlags = MaterialUpdateFlags::None; }

        // ---- Serialization for tooling ----

        /**
         * Serialize material parameters to JSON.
         */
        virtual auto serializeParameters(nlohmann::json& outJson) const -> void = 0;

        /**
         * Deserialize material parameters from JSON.
         * @return true if successful.
         */
        virtual auto deserializeParameters(nlohmann::json const& inJson) -> bool = 0;

    protected:
        MaterialUpdateFlags m_updateFlags{MaterialUpdateFlags::All};
    };

} // namespace april::graphics
