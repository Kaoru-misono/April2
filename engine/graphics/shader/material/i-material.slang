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
    MaterialType getType();

    IMaterialInstance setupMaterialInstance(
        MaterialSystemContext ms,
        ShadingData sd,
        ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    );

    IMaterialInstance setupDiffMaterialInstance(
        out DiffMaterialData diffData,
        MaterialSystemContext ms,
        ShadingData sd,
        ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    );

    VolumeProperties getHomogeneousVolumeProperties(MaterialSystemContext ms, uint materialID);
};
