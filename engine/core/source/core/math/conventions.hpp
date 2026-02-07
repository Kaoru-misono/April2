#pragma once

// Central math conventions for April2.
// Coordinate system: right-handed, +Y up, -Z forward.
// NDC depth range: [0, 1] (Vulkan/D3D style).
// Angles in radians.

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#ifndef GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_RIGHT_HANDED 1
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL 1
#endif

namespace april::inline math::conventions
{
    inline constexpr bool kRightHanded = true;
    inline constexpr bool kDepthZeroToOne = true;
    inline constexpr bool kForwardNegativeZ = true;
    inline constexpr bool kYUp = true;
}
