// Material factory extensions on MaterialSystem.

import material.material_types;
import material.material_data;
import material.i_material;
import material.i_material_instance;
import material.material_system;
import material.shading_data;
import material.texture_sampler;
import material.material_instance_hints;

extension MaterialSystem
{
    [ForceInline]
    IMaterial getMaterial(const uint materialID)
    {
        MaterialType type = getMaterialType(materialID);
        return createDynamicObject<IMaterial, MaterialDataBlob>(int(type), getMaterialDataBlob(materialID));
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
