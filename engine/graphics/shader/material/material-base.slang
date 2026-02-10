// Base defaults for IMaterial implementations.

import material.material_types;
import material.i_material;

struct MaterialBase
{
    MaterialType getType()
    {
        return MaterialType::Unknown;
    }
};
