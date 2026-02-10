// Material factory extensions on MaterialSystemContext.

import material.material_types;
import material.material_data;
import material.i_material;
import material.i_material_instance;
import material.material_system;
import material.shading_data;
import material.texture_sampler;
import material.material_instance_hints;

extension MaterialSystemContext
{
    [ForceInline]
    IMaterial getMaterial(const uint materialID)
    {
        MaterialType type = MaterialType(materials[materialID].header.materialType);
        return createDynamicObject<IMaterial, uint>(int(type), materialID);
    }

    [ForceInline]
    IMaterialInstance getMaterialInstance<L: ITextureSampler>(const ShadingData sd, const L lod, const uint hints = (uint)MaterialInstanceHints::None)
    {
        IMaterial material = getMaterial(sd.materialID);
        return material.setupMaterialInstance(this, sd, lod, hints);
    }

    [ForceInline]
    IMaterialInstance getMaterialInstance(const ShadingData sd)
    {
        ImplicitLodTextureSampler lod;
        return getMaterialInstance(sd, lod, (uint)MaterialInstanceHints::None);
    }
}
