import material.phase.i_phase_function;

struct HenyeyGreensteinPhaseFunction : IPhaseFunction
{
    float g;

    __init(float anisotropy = 0.0f)
    {
        g = anisotropy;
    }

    float eval(float cosTheta)
    {
        float gg = g * g;
        float denom = pow(max(1.0f + gg - 2.0f * g * cosTheta, 1e-6f), 1.5f);
        return (1.0f - gg) / (4.0f * 3.1415926535f * denom);
    }
};
