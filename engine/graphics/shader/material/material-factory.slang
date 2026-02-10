// Material factory helpers to create IMaterialInstance implementations.

import material.i_material;
import material.material_data;
import material.material_system;
import material.shading_data;
import material.standard_material;
import material.unlit_material;

struct MaterialFactory
{
    MaterialSystemContext ms;

    __init(MaterialSystemContext materialSystem)
    {
        ms = materialSystem;
    }

    IMaterialInstance getMaterialInstance(uint materialID, ShadingData sd)
    {
        return getMaterialInstanceFromSystem(ms, materialID, sd);
    }
};

IMaterialInstance getMaterialInstance(MaterialFactory factory, uint materialID, ShadingData sd)
{
    return factory.getMaterialInstance(materialID, sd);
}
