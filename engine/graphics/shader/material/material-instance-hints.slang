// Material instance behavior hints.

enum MaterialInstanceHints : uint
{
    None = 0,
    DisableNormalMapping = 1 << 0,
    AdjustShadingNormal = 1 << 1,
};

bool isNormalMappingEnabled(uint hints)
{
    return (hints & (uint)MaterialInstanceHints::DisableNormalMapping) == 0;
}

bool isAdjustShadingNormalEnabled(uint hints)
{
    return (hints & (uint)MaterialInstanceHints::AdjustShadingNormal) != 0;
}
