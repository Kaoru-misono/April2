// IMaterial interface - Abstract base class for materials.

#pragma once

#include "generated/material/material-types.generated.hpp"
#include "generated/material/material-data.generated.hpp"
#include "program/program.hpp"

#include <core/foundation/object.hpp>

namespace april::graphics
{
    class ShaderVariable;
    class Texture;

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
    };

} // namespace april::graphics
