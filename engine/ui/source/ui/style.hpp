#pragma once

#include <cmath>
#include <imgui.h>
#include <initializer_list>

namespace april::ui
{

    //--------------------------------------------------------------------------------------------------
    // Setting a dark style for the GUI
    // The colors were coded in sRGB color space, set the useLinearColor
    // flag to convert to linear color space.
    inline auto setupStyle(bool useLinearColor = true) -> void
    {
        auto srgb = [useLinearColor](float r, float g, float b, float a) -> ImVec4 {
            if (!useLinearColor)
            {
                return ImVec4(r, g, b, a);
            }
            auto toLinearScalar = [](float u) -> float {
                return u <= 0.04045f ? 25.0f * u / 323.0f : std::pow((200.0f * u + 11.0f) / 211.0f, 2.4f);
            };
            return ImVec4(toLinearScalar(r), toLinearScalar(g), toLinearScalar(b), a);
        };

        ImGui::StyleColorsDark();

        ImGuiStyle& style               = ImGui::GetStyle();
        style.WindowRounding            = 0.0f;
        style.WindowBorderSize          = 0.0f;
        style.ColorButtonPosition       = ImGuiDir_Right;
        style.FrameRounding             = 2.0f;
        style.FrameBorderSize           = 1.0f;
        style.GrabRounding              = 4.0f;
        style.IndentSpacing             = 12.0f;
        style.Colors[ImGuiCol_WindowBg] = srgb(0.2f, 0.2f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = srgb(0.2f, 0.2f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = srgb(0.2f, 0.2f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_PopupBg]     = srgb(0.135f, 0.135f, 0.135f, 1.0f);
        style.Colors[ImGuiCol_Border]      = srgb(0.4f, 0.4f, 0.4f, 0.5f);
        style.Colors[ImGuiCol_FrameBg]     = srgb(0.05f, 0.05f, 0.05f, 0.5f);

        // Normal
        ImVec4 normal_color = srgb(0.465f, 0.465f, 0.525f, 1.0f);
        for (auto c : {ImGuiCol_Header, ImGuiCol_SliderGrab, ImGuiCol_Button, ImGuiCol_CheckMark, ImGuiCol_ResizeGrip, ImGuiCol_TextSelectedBg, ImGuiCol_Separator, ImGuiCol_FrameBgActive})
        {
            style.Colors[c] = normal_color;
        }

        // Active
        ImVec4 active_color = srgb(0.365f, 0.365f, 0.425f, 1.0f);
        for (auto c : {ImGuiCol_HeaderActive, ImGuiCol_SliderGrabActive, ImGuiCol_ButtonActive, ImGuiCol_ResizeGripActive, ImGuiCol_SeparatorActive})
        {
            style.Colors[c] = active_color;
        }

        // Hovered
        ImVec4 hovered_color = srgb(0.565f, 0.565f, 0.625f, 1.0f);
        for (auto c : {ImGuiCol_HeaderHovered, ImGuiCol_ButtonHovered, ImGuiCol_FrameBgHovered, ImGuiCol_ResizeGripHovered, ImGuiCol_SeparatorHovered})
        {
            style.Colors[c] = hovered_color;
        }

        style.Colors[ImGuiCol_TitleBgActive]    = srgb(0.465f, 0.465f, 0.465f, 1.0f);
        style.Colors[ImGuiCol_TitleBg]          = srgb(0.125f, 0.125f, 0.125f, 1.0f);
        style.Colors[ImGuiCol_Tab]              = srgb(0.05f, 0.05f, 0.05f, 0.5f);
        style.Colors[ImGuiCol_TabHovered]       = srgb(0.465f, 0.495f, 0.525f, 1.0f);
        style.Colors[ImGuiCol_TabActive]        = srgb(0.282f, 0.290f, 0.302f, 1.0f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = srgb(0.465f, 0.465f, 0.465f, 0.350f);

        ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
    }

} // namespace april::ui
