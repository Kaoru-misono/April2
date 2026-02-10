// MaterialSystem - Manages materials and GPU buffers for rendering.

#pragma once

#include "i-material.hpp"
#include "material-type-registry.hpp"
#include "rhi/buffer.hpp"
#include "rhi/sampler.hpp"
#include "rhi/texture.hpp"
#include "generated/material/material-data.generated.hpp"
#include "program/define-list.hpp"

#include <vector>
#include <unordered_map>
#include <map>

namespace april::graphics
{
    class Device;
    class ShaderVariable;

    /**
     * Descriptor table configuration for the material system.
     * These values must match between host and shader.
     */
    struct MaterialSystemConfig
    {
        uint32_t textureTableSize = 128;
        uint32_t samplerTableSize = 8;
        uint32_t bufferTableSize = 16;
    };

    /**
     * Material system diagnostics for debugging and profiling.
     */
    struct MaterialSystemDiagnostics
    {
        uint32_t totalMaterialCount = 0;
        uint32_t standardMaterialCount = 0;
        uint32_t unlitMaterialCount = 0;
        uint32_t otherMaterialCount = 0;

        uint32_t textureDescriptorCount = 0;
        uint32_t textureDescriptorCapacity = 0;
        uint32_t samplerDescriptorCount = 0;
        uint32_t samplerDescriptorCapacity = 0;
        uint32_t bufferDescriptorCount = 0;
        uint32_t bufferDescriptorCapacity = 0;

        uint32_t textureOverflowCount = 0;
        uint32_t samplerOverflowCount = 0;
        uint32_t bufferOverflowCount = 0;
        uint32_t invalidHandleCount = 0;
    };

    /**
     * Material system for managing materials and their GPU data.
     *
     * Responsibilities:
     * - Store and manage all materials
     * - Maintain GPU buffer with material data
     * - Aggregate type conformances from all materials
     * - Bind material data to shaders
     */
    class MaterialSystem : public core::Object
    {
        APRIL_OBJECT(MaterialSystem)
    public:
        explicit MaterialSystem(core::ref<Device> device, MaterialSystemConfig config = {});
        ~MaterialSystem() override = default;

        /**
         * Get shader defines for descriptor table capacities.
         * Pass these to shader compilation to synchronize host/shader limits.
         */
        auto getShaderDefines() const -> DefineList;

        /**
         * Get configuration for this material system.
         */
        auto getConfig() const -> MaterialSystemConfig const& { return m_config; }

        /**
         * Get current diagnostics snapshot.
         */
        auto getDiagnostics() const -> MaterialSystemDiagnostics;

        /**
         * Add a material to the system.
         * @param material Material to add.
         * @return Index of the material in the GPU buffer.
         */
        auto addMaterial(core::ref<IMaterial> material) -> uint32_t;

        /**
         * Remove a material from the system.
         * @param index Index of the material to remove.
         */
        auto removeMaterial(uint32_t index) -> void;

        /**
         * Get a material by index.
         * @param index Material index.
         * @return Material at the given index, or null if not found.
         */
        auto getMaterial(uint32_t index) const -> core::ref<IMaterial>;

        /**
         * Get the total number of materials.
         */
        auto getMaterialCount() const -> uint32_t;

        /**
         * Update GPU buffers with current material data.
         * Call this after modifying material properties.
         */
        auto updateGpuBuffers() -> void;

        /**
         * Bind material data buffer to a shader variable.
         * @param var Shader variable for the material data buffer.
         */
        auto bindToShader(ShaderVariable& var) const -> void;

        /**
         * Get the material data buffer.
         */
        auto getMaterialDataBuffer() const -> core::ref<Buffer>;

        /**
         * Get aggregated type conformances from all materials.
         * Call this when setting up shader compilation.
         */
        auto getTypeConformances() const -> TypeConformanceList;
        auto getTypeConformances(generated::MaterialType type) const -> TypeConformanceList;
        auto getShaderModules() const -> ProgramDesc::ShaderModuleList;
        auto getShaderModules(ProgramDesc::ShaderModuleList& modules) const -> void;

        using DescriptorHandle = uint32_t;
        static constexpr DescriptorHandle kInvalidDescriptorHandle = 0;

        auto registerTextureDescriptor(core::ref<Texture> texture) -> DescriptorHandle;
        auto registerSamplerDescriptor(core::ref<Sampler> sampler) -> DescriptorHandle;
        auto registerBufferDescriptor(core::ref<Buffer> buffer) -> DescriptorHandle;

        auto getTextureDescriptorResource(DescriptorHandle handle) const -> core::ref<Texture>;
        auto getSamplerDescriptorResource(DescriptorHandle handle) const -> core::ref<Sampler>;
        auto getBufferDescriptorResource(DescriptorHandle handle) const -> core::ref<Buffer>;
        auto getMaterialTypeRegistry() const -> MaterialTypeRegistry const&;
        auto getMaterialTypeId(uint32_t materialIndex) const -> uint32_t;

        /**
         * Mark the system as needing an update.
         * Called automatically when materials are added/removed.
         */
        auto markDirty() -> void;

        /**
         * Check if the system needs updating.
         */
        auto isDirty() const -> bool;

    private:
        core::ref<Device> m_device;
        MaterialSystemConfig m_config{};

        std::vector<core::ref<IMaterial>> m_materials;
        std::unordered_map<IMaterial*, uint32_t> m_materialIndices;

        core::ref<Buffer> m_materialDataBuffer;
        std::vector<generated::StandardMaterialData> m_cpuMaterialData;
        MaterialTypeRegistry m_materialTypeRegistry{};

        std::vector<core::ref<Texture>> m_textureDescriptors;
        std::vector<core::ref<Sampler>> m_samplerDescriptors;
        std::vector<core::ref<Buffer>> m_bufferDescriptors;
        std::unordered_map<Texture*, DescriptorHandle> m_textureDescriptorIndices;
        std::unordered_map<Sampler*, DescriptorHandle> m_samplerDescriptorIndices;
        std::unordered_map<Buffer*, DescriptorHandle> m_bufferDescriptorIndices;

        // Overflow tracking for diagnostics.
        mutable uint32_t m_textureOverflowCount{0};
        mutable uint32_t m_samplerOverflowCount{0};
        mutable uint32_t m_bufferOverflowCount{0};
        mutable uint32_t m_invalidHandleCount{0};

        bool m_dirty{true};
        mutable bool m_codeCacheDirty{true};

        mutable std::map<generated::MaterialType, TypeConformanceList> m_typeConformancesByType;
        mutable ProgramDesc::ShaderModuleList m_shaderModules;

        static constexpr uint32_t kInitialBufferCapacity = 64;

        auto ensureBufferCapacity(uint32_t requiredCount) -> void;
        auto writeMaterialData(uint32_t index) -> void;
        auto rebuildMaterialData() -> void;
        auto validateAndClampDescriptorHandle(
            uint32_t& handle,
            uint32_t maxCount,
            uint32_t& overflowCounter,
            uint32_t materialIndex,
            char const* slotName
        ) const -> void;
        auto rebuildCodeCache() const -> void;
    };

} // namespace april::graphics
