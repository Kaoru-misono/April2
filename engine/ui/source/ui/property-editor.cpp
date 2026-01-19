#include "property-editor.hpp"
#include "tools/tooltip.hpp"

#include <algorithm>
#include <cstdarg>

namespace april::ui::PropertyEditor
{

    template<typename T>
    auto Clamped(bool changed, T* value, T min, T max) -> bool
    {
        *value = std::max(min, std::min(max, *value));
        return changed;
    }

    // Beginning the Property Editor
    auto begin(const char* label, ImGuiTableFlags flag) -> bool
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        bool result = ImGui::BeginTable(label, 2, flag);
        if (!result)
        {
            ImGui::PopStyleVar();
        }
        return result;
    }

    // Generic entry, the lambda function should return true if the widget changed
    auto entry(const std::string& property_name, const std::function<bool()>& content_fct, const std::string& tooltip) -> bool
    {
        ImGui::PushID(property_name.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", property_name.c_str());
        if (!tooltip.empty())
            april::ui::Tooltip::hover(tooltip.c_str(), false, 0.0f);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        bool result = content_fct();
        if (!tooltip.empty())
            april::ui::Tooltip::hover(tooltip.c_str());
        ImGui::PopID();
        return result; // returning if the widget changed
    }

    // Text specialization
    auto entry(const std::string& property_name, const std::string& value) -> void
    {
        entry(property_name, [&] {
            ImGui::Text("%s", value.c_str());
            return false; // dummy, no change
        });
    }

    auto treeNode(const std::string& name, ImGuiTreeNodeFlags flags) -> bool
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        return ImGui::TreeNodeEx(name.c_str(), flags);
    }
    auto treePop() -> void
    {
        ImGui::TreePop();
    }

    // Ending the Editor
    auto end() -> void
    {
        ImGui::EndTable();
        ImGui::PopStyleVar();
    }

    auto Button(const char* label, const ImVec2& size, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::Button("##hidden", size); }, tooltip);
    }
    auto SmallButton(const char* label, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SmallButton("##hidden"); }, tooltip);
    }
    auto Checkbox(const char* label, bool* v, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::Checkbox("##hidden", v); }, tooltip);
    }
    auto RadioButton(const char* label, bool active, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::RadioButton("##hidden", active); }, tooltip);
    }
    auto RadioButton(const char* label, int* v, int v_button, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::RadioButton("##hidden", v, v_button); }, tooltip);
    }
    auto Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items, const std::string& tooltip) -> bool
    {
        return entry(label,
                     [&] { return ImGui::Combo("##hidden", current_item, items, items_count, popup_max_height_in_items); });
    }
    auto Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items, const std::string& tooltip) -> bool
    {
        return entry(
            label, [&] { return ImGui::Combo("##hidden", current_item, items_separated_by_zeros, popup_max_height_in_items); }, tooltip);
    }
    auto Combo(const char* label,
               int* current_item,
               const char* (*getter)(void* user_data, int idx),
               void* user_data,
               int items_count,
               int popup_max_height_in_items,
               const std::string& tooltip) -> bool
    {
        return entry(
            label,
            [&] { return ImGui::Combo("##hidden", current_item, getter, user_data, items_count, popup_max_height_in_items); }, tooltip);
    }
    auto SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderFloat("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderFloat2("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderFloat3("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderFloat4("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderAngle(const char* label,
                     float* v_rad,
                     float v_degrees_min,
                     float v_degrees_max,
                     const char* format,
                     ImGuiSliderFlags flags,
                     const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderAngle("##hidden", v_rad, v_degrees_min, v_degrees_max, format, flags); }, tooltip);
    }
    auto SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderInt("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderInt2("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderInt3("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderInt4("##hidden", v, v_min, v_max, format, flags); }, tooltip);
    }
    auto SliderScalar(const char* label,
                      ImGuiDataType data_type,
                      void* p_data,
                      const void* p_min,
                      const void* p_max,
                      const char* format,
                      ImGuiSliderFlags flags,
                      const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::SliderScalar("##hidden", data_type, p_data, p_min, p_max, format, flags); }, tooltip);
    }

    auto DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragFloat("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragFloat2("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragFloat3("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragFloat4("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragInt("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragInt2("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragInt3("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::DragInt4("##hidden", v, v_speed, v_min, v_max, format, flags); }, tooltip);
    }
    auto DragScalar(const char* label,
                    ImGuiDataType data_type,
                    void* p_data,
                    float v_speed,
                    const void* p_min,
                    const void* p_max,
                    const char* format,
                    ImGuiSliderFlags flags,
                    const std::string& tooltip) -> bool
    {
        return entry(
            label, [&] { return ImGui::DragScalar("##hidden", data_type, p_data, v_speed, p_min, p_max, format, flags); }, tooltip);
    }
    auto InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputText("##hidden", buf, buf_size, flags); }, tooltip);
    }
    auto InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputTextMultiline("##hidden", buf, buf_size, size, flags); }, tooltip);
    }
    auto InputFloat(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        float tv = *v;
        bool changed = entry(
            label,
            [&] {
                return ImGui::InputFloat("##hidden", &tv, step, step_fast, format, (flags & ~ImGuiInputTextFlags_EnterReturnsTrue));
            },
            tooltip);

        if (changed && (!(flags & ImGuiInputTextFlags_EnterReturnsTrue) || (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemClicked())))
        {
            *v = tv;
            return true;
        }
        else
        {
            return false;
        }
    }
    auto InputFloat2(const char* label, float v[2], const char* format, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputFloat2("##hidden", v, format, flags); }, tooltip);
    }
    auto InputFloat3(const char* label, float v[3], const char* format, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputFloat3("##hidden", v, format, flags); }, tooltip);
    }
    auto InputFloat4(const char* label, float v[4], const char* format, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputFloat4("##hidden", v, format, flags); }, tooltip);
    }
    auto InputInt(const char* label, int* v, int step, int step_fast, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        int tv = *v;
        bool changed = entry(
            label,
            [&] { return ImGui::InputInt("##hidden", &tv, step, step_fast, (flags & ~ImGuiInputTextFlags_EnterReturnsTrue)); }, tooltip);

        if (changed && (!(flags & ImGuiInputTextFlags_EnterReturnsTrue) || (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemClicked())))
        {
            *v = tv;
            return true;
        }
        else
        {
            return false;
        }
    }
    auto InputIntClamped(const char* label, int* v, int min, int max, int step, int step_fast, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        int tv = *v;
        bool changed = entry(
            label,
            [&] { return ImGui::InputInt("##hidden", &tv, step, step_fast, (flags & ~ImGuiInputTextFlags_EnterReturnsTrue)); }, tooltip);

        changed = changed && (!(flags & ImGuiInputTextFlags_EnterReturnsTrue) || (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemClicked()));
        if (changed)
        {
            *v = tv;
        }
        return Clamped(changed, v, min, max);
    }
    auto InputInt2(const char* label, int v[2], ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputInt2("##hidden", v, flags); }, tooltip);
    }
    auto InputInt3(const char* label, int v[3], ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputInt3("##hidden", v, flags); }, tooltip);
    }
    auto InputInt4(const char* label, int v[4], ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputInt4("##hidden", v, flags); }, tooltip);
    }
    auto InputDouble(const char* label, double* v, double step, double step_fast, const char* format, ImGuiInputTextFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::InputDouble("##hidden", v, step, step_fast, format, flags); }, tooltip);
    }
    auto InputScalar(const char* label,
                     ImGuiDataType data_type,
                     void* p_data,
                     const void* p_step,
                     const void* p_step_fast,
                     const char* format,
                     ImGuiInputTextFlags flags,
                     const std::string& tooltip) -> bool
    {
        return entry(
            label, [&] { return ImGui::InputScalar("##hidden", data_type, p_data, p_step, p_step_fast, format, flags); }, tooltip);
    }

    auto ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::ColorEdit3("##hidden", col, flags); }, tooltip);
    }
    auto ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::ColorEdit4("##hidden", col, flags); }, tooltip);
    }
    auto ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::ColorPicker3("##hidden", col, flags); }, tooltip);
    }
    auto ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::ColorPicker4("##hidden", col, flags); }, tooltip);
    }
    auto ColorButton(const char* label, const ImVec4& col, ImGuiColorEditFlags flags, const ImVec2& size, const std::string& tooltip) -> bool
    {
        return entry(label, [&] { return ImGui::ColorButton("##hidden", col, flags, size); }, tooltip);
    }
    auto Text(const char* label, const std::string& text) -> void
    {
        entry(label, [&] {
            ImGui::Text("%s", text.c_str());
            return false; // dummy, no change
        });
    }
    auto Text(const char* label, const char* fmt, ...) -> void
    {
        va_list args;
        va_start(args, fmt);
        entry(label, [&] {
            ImGui::TextV(fmt, args);
            return false; // dummy, no change
        });
        va_end(args);
    }

} // namespace april::ui::PropertyEditor
