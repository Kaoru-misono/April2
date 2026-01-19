#pragma once

#include <glm/glm.hpp>
#include "../../shader/tonemap_io.h.slang"

namespace april::ui
{
    // Displays ImGui UI for shaderio::TonemapperData properties.
    // Returns whether any fields changed.
    auto tonemapperWidget(shaderio::TonemapperData& tonemapper) -> bool;

} // namespace april::ui
