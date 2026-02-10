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
    uint materialID;
    typedef UnlitMaterialInstance MaterialInstance;

    __init(uint materialID_)
    {
        materialID = materialID_;
    }

    MaterialType getType()
    {
        return MaterialType::Unlit;
    }

    UnlitMaterialInstance setupMaterialInstance(
        const MaterialSystemContext ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        StandardMaterialData data = ms.getMaterialData(materialID);
        return UnlitMaterialInstance(data);
    }

    UnlitMaterialInstance setupDiffMaterialInstance(
        out DiffMaterialData diffData,
        const MaterialSystemContext ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        diffData.payload = 0.0f;
        return setupMaterialInstance(ms, sd, lod, hints);
    }

    VolumeProperties getHomogeneousVolumeProperties(const MaterialSystemContext ms, const uint materialID)
    {
        VolumeProperties p;
        p.sigmaA = 0.0f;
        p.sigmaS = 0.0f;
        p.g = 0.0f;
        return p;
    }
};
