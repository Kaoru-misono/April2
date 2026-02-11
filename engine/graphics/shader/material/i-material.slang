// IMaterial interface - high-level runtime material abstraction.

import material.material_types;
import material.shading_data;
import material.material_system;
import material.i_material_instance;
import material.texture_sampler;
import material.material_instance_hints;

[anyValueSize(128)]
interface IMaterial
{
    associatedtype MaterialInstance : IMaterialInstance;

    MaterialType getType();

    MaterialInstance setupMaterialInstance(
        const MaterialSystem ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    );

    MaterialInstance setupDiffMaterialInstance(
        out DiffMaterialData diffData,
        const MaterialSystem ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    );

    VolumeProperties getHomogeneousVolumeProperties(const MaterialSystem ms, const uint materialID);
};
