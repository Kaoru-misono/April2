#pragma once

#include <imgui.h>

#include <cstddef>
#include <string_view>

namespace april::editor::ui
{
    inline auto Button(char const* label) -> bool
    {
        return ImGui::Button(label);
    }

    inline auto Label(std::string_view text) -> void
    {
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
    }

    inline auto Image(ImTextureID texture, ImVec2 size, ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f)) -> void
    {
        ImGui::Image(texture, size, uv0, uv1);
    }

    class Toolbar
    {
    public:
        auto inputTextWithHint(char const* label, char const* hint, char* buffer, size_t bufferSize, float width) -> bool
        {
            nextItem();
            if (width > 0.0f)
            {
                ImGui::SetNextItemWidth(width);
            }
            return ImGui::InputTextWithHint(label, hint, buffer, bufferSize);
        }

        auto combo(char const* label, int* currentItem, char const* const items[], int itemCount, float width) -> bool
        {
            nextItem();
            if (width > 0.0f)
            {
                ImGui::SetNextItemWidth(width);
            }
            return ImGui::Combo(label, currentItem, items, itemCount);
        }

        auto sliderFloat(char const* label, float* value, float minValue, float maxValue, char const* format, float width) -> bool
        {
            nextItem();
            if (width > 0.0f)
            {
                ImGui::SetNextItemWidth(width);
            }
            return ImGui::SliderFloat(label, value, minValue, maxValue, format);
        }

        auto button(char const* label, char const* tooltip = nullptr) -> bool
        {
            nextItem();
            auto pressed = ImGui::Button(label);
            showTooltip(tooltip);
            return pressed;
        }

        auto toggleButton(char const* labelOn, char const* labelOff, bool* value, char const* tooltipOn = nullptr, char const* tooltipOff = nullptr) -> bool
        {
            nextItem();
            auto pressed = ImGui::Button(*value ? labelOn : labelOff);
            showTooltip(*value ? tooltipOn : tooltipOff);
            if (pressed)
            {
                *value = !*value;
            }
            return pressed;
        }

        auto checkbox(char const* label, bool* value, char const* tooltip = nullptr) -> bool
        {
            nextItem();
            auto changed = ImGui::Checkbox(label, value);
            showTooltip(tooltip);
            return changed;
        }

        auto textFilter(ImGuiTextFilter& filter, float width) -> void
        {
            nextItem();
            filter.Draw("Filter", width);
        }

    private:
        auto nextItem() -> void
        {
            if (!m_first)
            {
                ImGui::SameLine();
            }
            m_first = false;
        }

        static auto showTooltip(char const* tooltip) -> void
        {
            if (tooltip && ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", tooltip);
            }
        }

        bool m_first{true};
    };
}
