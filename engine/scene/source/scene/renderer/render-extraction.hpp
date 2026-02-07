#pragma once

#include "render-resource-registry.hpp"
#include "render-types.hpp"

#include <scene/components.hpp>
#include <scene/ecs-core.hpp>

namespace april::scene
{
    auto extractFrameSnapshot(Registry& registry, RenderResourceRegistry& resources, FrameSnapshot& snapshot) -> void;
}
