#pragma once
#include <functional>
#include <string>
#include <imgui.h>

namespace april::ui::PropertyEditor
{

    auto begin(const char* label = "PE::Table", ImGuiTableFlags flag = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable) -> bool;
    auto end() -> void;
    auto entry(const std::string& property_name, const std::function<bool()>& content_fct, const std::string& tooltip = {}) -> bool;
    auto entry(const std::string& property_name, const std::string& value) -> void;
    auto treeNode(const std::string& name, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth) -> bool;
    auto treePop() -> void;

    auto Button(const char* label, const ImVec2& size = ImVec2(0, 0), const std::string& tooltip = {}) -> bool;
    auto SmallButton(const char* label, const std::string& tooltip = {}) -> bool;
    auto Checkbox(const char* label, bool* v, const std::string& tooltip = {}) -> bool;
    auto RadioButton(const char* label, bool active, const std::string& tooltip = {}) -> bool;
    auto RadioButton(const char* label, int* v, int v_button, const std::string& tooltip = {}) -> bool;
    auto Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1, const std::string& tooltip = {}) -> bool;
    auto Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1, const std::string& tooltip = {}) -> bool;
    auto Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items = -1, const std::string& tooltip = {}) -> bool;
    auto SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = NULL, ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragFloat2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragFloat3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragFloat4(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragInt(const char* label, int* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragInt2(const char* label, int v[2], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragInt3(const char* label, int v[3], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragInt4(const char* label, int v[4], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed = 1.0f, const void* p_min = NULL, const void* p_max = NULL, const char* format = NULL, ImGuiSliderFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputFloat2(const char* label, float v[2], const char* format = "%.3f", ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputFloat3(const char* label, float v[3], const char* format = "%.3f", ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputFloat4(const char* label, float v[4], const char* format = "%.3f", ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputInt(const char* label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputIntClamped(const char* label, int* v, int min, int max, int step, int step_fast, ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputInt2(const char* label, int v[2], ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputInt3(const char* label, int v[3], ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputInt4(const char* label, int v[4], ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto InputScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags = 0, const std::string& tooltip = {}) -> bool;
    auto ColorButton(const char* label, const ImVec4& col, ImGuiColorEditFlags flags = 0, const ImVec2& size = ImVec2(0, 0), const std::string& tooltip = {}) -> bool;
    auto Text(const char* label, const std::string& text) -> void;
    auto Text(const char* label, const char* fmt, ...) -> void;

} // namespace april::ui::PropertyEditor