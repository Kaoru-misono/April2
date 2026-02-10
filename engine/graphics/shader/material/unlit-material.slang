import material.material_types;
import material.material_data;
import material.shading_data;
import material.material_system;
import material.i_material;
import material.i_material_instance;
import material.texture_sampler;
import material.material_instance_hints;
import material.unlit_material_instance;

struct UnlitMaterial : IMaterial
{
    StandardMaterialData data;

    __init(StandardMaterialData data_)
    {
        data = data_;
    }

    MaterialType getType()
    {
        return MaterialType::Unlit;
    }

    IMaterialInstance setupMaterialInstance(
        MaterialSystemContext ms,
        ShadingData sd,
        ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        return UnlitMaterialInstance(data);
    }

    IMaterialInstance setupDiffMaterialInstance(
        out DiffMaterialData diffData,
        MaterialSystemContext ms,
        ShadingData sd,
        ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        diffData.payload = 0.0f;
        return setupMaterialInstance(ms, sd, lod, hints);
    }

    VolumeProperties getHomogeneousVolumeProperties(MaterialSystemContext ms, uint materialID)
    {
        VolumeProperties p;
        p.sigmaA = 0.0f;
        p.sigmaS = 0.0f;
        p.g = 0.0f;
        return p;
    }
};
