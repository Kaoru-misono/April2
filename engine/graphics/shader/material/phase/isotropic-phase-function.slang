import material.phase.i_phase_function;

struct IsotropicPhaseFunction : IPhaseFunction
{
    float eval(float cosTheta)
    {
        (void)cosTheta;
        return 0.0795774715f;
    }
};
