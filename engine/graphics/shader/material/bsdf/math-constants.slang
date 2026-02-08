// Mathematical constants for BSDF calculations.

static const float M_PI = 3.14159265358979323846f;
static const float M_1_PI = 0.31830988618379067154f;  // 1/pi
static const float M_2_PI = 0.63661977236758134308f;  // 2/pi
static const float M_PI_2 = 1.57079632679489661923f;  // pi/2
static const float M_PI_4 = 0.78539816339744830962f;  // pi/4

static const float FLT_MAX = 3.402823466e+38f;
static const float FLT_MIN = 1.175494351e-38f;
static const float FLT_EPSILON = 1.192092896e-07f;

// Minimum cos(theta) for the incident and outgoing vectors.
// Some BSDF functions are not robust for cos(theta) == 0.0.
static const float kMinCosTheta = 1e-6f;

// Minimum GGX alpha to avoid numerical instability.
// Alpha below this value means using delta reflection/transmission.
static const float kMinGGXAlpha = 0.0064f;
