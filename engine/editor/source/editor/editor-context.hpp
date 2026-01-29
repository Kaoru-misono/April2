#pragma once

#include <core/math/type.hpp>
#include <string>

namespace april::editor
{
    struct EditorSelection
    {
        std::string entityName{};
    };

    struct EditorSceneRef
    {
        std::string name{"Scene"};
    };

    struct EditorToolState
    {
        enum class Tool
        {
            Select,
            Move,
            Rotate,
            Scale
        };

        Tool activeTool{Tool::Select};
        bool showStats{false};
    };

    struct EditorContext
    {
        float2 viewportSize{};
        std::string projectName{"April"};
        EditorSceneRef scene{};
        EditorSelection selection{};
        EditorToolState tools{};
    };
}
