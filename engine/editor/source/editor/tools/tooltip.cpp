#include "tooltip.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace april::ui
{

    auto Tooltip::hover(char const* description, bool questionMark, float timerThreshold) -> void
    {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (!ctx)
            return;

        bool passTimer = ctx->HoveredIdTimer >= timerThreshold && ctx->ActiveIdTimer == 0.0f;
        if (questionMark)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            passTimer = true;
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && passTimer)
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(description);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    auto Tooltip::property(char const* propertyName, char const* description) -> void
    {
        ImGui::Text("%s", propertyName);
        hover(description, true);
    }

} // namespace april::ui
