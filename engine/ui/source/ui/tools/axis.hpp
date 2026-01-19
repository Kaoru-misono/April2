#pragma once

#include <imgui.h>
#include <glm/glm.hpp>

namespace april::ui
{
    class Axis
    {
    public:
        // This utility is adding the 3D axis at `pos`, using the matrix `modelView`
        static auto render(ImVec2 pos, const glm::mat4& modelView, float size = 20.f) -> void;

        // Place the axis at the bottom left corner of the window
        static auto render(const glm::mat4& modelView, float size = 20.f) -> void
        {
            ImVec2 windowPos  = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 offset     = ImVec2(size * 1.1f * ImGui::GetWindowDpiScale(), -size * 1.1f * ImGui::GetWindowDpiScale());
            ImVec2 pos        = ImVec2(windowPos.x + offset.x, windowPos.y + windowSize.y + offset.y);
            render(pos, modelView, size);
        }
    };

} // namespace april::ui