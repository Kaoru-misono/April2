// Material factory extension API.

import material.i_material;
import material.material_system;
import material.shading_data;
import material.texture_sampler;
import material.material_instance_hints;

IMaterial getMaterial(MaterialSystemContext ms, uint materialID)
{
    return getMaterialObject(ms, materialID);
}

IMaterialInstance getMaterialInstance(MaterialSystemContext ms, uint materialID, ShadingData sd)
{
    ImplicitLodTextureSampler lod;
    return getMaterialInstanceFromSystem(ms, materialID, sd, lod, (uint)MaterialInstanceHints::None);
}

IMaterialInstance getMaterialInstance<L: ITextureSampler>(
    MaterialSystemContext ms,
    uint materialID,
    ShadingData sd,
    L lod,
    uint hints = (uint)MaterialInstanceHints::None
)
{
    return getMaterialInstanceFromSystem(ms, materialID, sd, lod, hints);
}
