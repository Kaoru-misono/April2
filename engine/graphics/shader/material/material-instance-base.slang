// Base defaults for IMaterialInstance implementations.

import material.i_material_instance;

struct MaterialInstanceBase
{
    bool hasVolumeProperties()
    {
        return false;
    }

    VolumeProperties getVolumeProperties()
    {
        VolumeProperties p;
        p.sigmaA = 0.0f;
        p.sigmaS = 0.0f;
        p.g = 0.0f;
        return p;
    }
};
