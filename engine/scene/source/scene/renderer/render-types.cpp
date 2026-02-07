#include "render-types.hpp"

namespace april::scene
{
    auto FrameSnapshot::reset() -> void
    {
        mainView = {};
        staticMeshes.clear();
        dynamicMeshes.clear();
        lights.clear();
    }
}
