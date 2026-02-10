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
    IMaterial getMaterial(uint materialID)
    {
        MaterialType type = MaterialType(materials[materialID].header.materialType);
        return createDynamicObject<IMaterial, StandardMaterialData>(int(type), materials[materialID]);
    }

    IMaterialInstance getMaterialInstance<L: ITextureSampler>(ShadingData sd, L lod, uint hints = (uint)MaterialInstanceHints::None)
    {
        IMaterial material = getMaterial(sd.materialID);
        return material.setupMaterialInstance(this, sd, lod, hints);
    }

    IMaterialInstance getMaterialInstance(ShadingData sd)
    {
        ImplicitLodTextureSampler lod;
        return getMaterialInstance(sd, lod, (uint)MaterialInstanceHints::None);
    }
}
