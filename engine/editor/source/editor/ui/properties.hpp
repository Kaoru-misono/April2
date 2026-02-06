#pragma once

#include "scoped.hpp"

#include <editor/editor-context.hpp>
#include <core/math/type.hpp>
#include <imgui.h>

#include <string>
#include <type_traits>
#include <utility>

namespace april::editor::ui
{
    struct PropertyOptions
    {
        float speed{0.1f};
        float min{0.0f};
        float max{0.0f};
        float reset{0.0f};
        char const* format{nullptr};
        ImGuiInputTextFlags textFlags{};
    };

    auto Property(char const* label, int& value, PropertyOptions options) -> bool;
    auto Property(char const* label, float& value, PropertyOptions options) -> bool;
    auto Property(char const* label, bool& value, PropertyOptions options) -> bool;
    auto Property(char const* label, std::string& value, PropertyOptions options) -> bool;
    auto Property(char const* label, float3& value, PropertyOptions options) -> bool;

    namespace detail
    {
        template <typename ComponentType>
        inline auto markDirtyIfPresent(ComponentType& component) -> void
        {
            if constexpr (requires { component.isDirty; })
            {
                component.isDirty = true;
            }
        }

        inline auto inputTextResize(ImGuiInputTextCallbackData* data) -> int
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                auto* value = static_cast<std::string*>(data->UserData);
                value->resize(static_cast<size_t>(data->BufTextLen));
                data->Buf = value->data();
            }
            return 0;
        }

        inline auto vec3Control(char const* label, float3& value, PropertyOptions options) -> bool
        {
            auto const* format = options.format ? options.format : "%.3f";
            bool changed = false;

            auto const lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
            auto const buttonSize = ImVec2(lineHeight + 4.0f, lineHeight);

            ui::ScopedID scope{label};
            ui::ScopedTable table{"##vec3", 7, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX};
            if (!table)
            {
                return false;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            auto drawAxis = [&](int buttonColumn, int valueColumn, char const* axisLabel, char const* dragLabel, float& component, ImVec4 color) {
                auto hovered = ImVec4(color.x + 0.1f, color.y + 0.1f, color.z + 0.1f, color.w);
                auto active = ImVec4(color.x + 0.2f, color.y + 0.2f, color.z + 0.2f, color.w);

                ImGui::TableSetColumnIndex(buttonColumn);
                ui::ScopedStyleColor axisColors{
                    ImGuiCol_Button, color,
                    ImGuiCol_ButtonHovered, hovered,
                    ImGuiCol_ButtonActive, active
                };
                if (ImGui::Button(axisLabel, buttonSize))
                {
                    component = options.reset;
                    changed = true;
                }

                ImGui::TableSetColumnIndex(valueColumn);
                ImGui::SetNextItemWidth(80.0f);
                if (ImGui::DragFloat(dragLabel, &component, options.speed, options.min, options.max, format))
                {
                    changed = true;
                }
            };

            drawAxis(1, 2, "X", "##X", value.x, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            drawAxis(3, 4, "Y", "##Y", value.y, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            drawAxis(5, 6, "Z", "##Z", value.z, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));

            return changed;
        }

        template <typename T, typename ApplyFn>
        auto undoableValue(EditorContext& context,
            char const* label,
            T& value,
            char const* action,
            ApplyFn&& applyFn,
            PropertyOptions options) -> bool
        {
            auto edited = value;
            if (!ui::Property(label, edited, options))
            {
                return false;
            }

            auto oldValue = value;
            context.commandStack.apply(
                action,
                oldValue,
                edited,
                [applyFn = std::forward<ApplyFn>(applyFn)](T const& v) mutable { applyFn(v); }
            );
            return true;
        }
    }

    inline auto Property(char const* label, int& value, PropertyOptions options = {}) -> bool
    {
        auto const* format = options.format ? options.format : "%d";
        return ImGui::DragInt(label, &value, options.speed,
            static_cast<int>(options.min),
            static_cast<int>(options.max),
            format);
    }

    inline auto Property(char const* label, float& value, PropertyOptions options = {}) -> bool
    {
        auto const* format = options.format ? options.format : "%.3f";
        return ImGui::DragFloat(label, &value, options.speed, options.min, options.max, format);
    }

    inline auto Property(char const* label, bool& value, PropertyOptions /*options*/ = {}) -> bool
    {
        return ImGui::Checkbox(label, &value);
    }

    inline auto Property(char const* label, std::string& value, PropertyOptions options = {}) -> bool
    {
        if (value.capacity() == 0)
        {
            value.reserve(64);
        }
        auto flags = options.textFlags | ImGuiInputTextFlags_CallbackResize;
        return ImGui::InputText(label, value.data(), value.capacity() + 1, flags, detail::inputTextResize, &value);
    }

    inline auto Property(char const* label, float3& value, PropertyOptions options = {}) -> bool
    {
        return detail::vec3Control(label, value, options);
    }

    template <typename ApplyFn>
    inline auto PropertyUndoable(EditorContext& context, char const* label, bool& value, char const* action, ApplyFn&& applyFn) -> bool
    {
        return detail::undoableValue(context, label, value, action, std::forward<ApplyFn>(applyFn), {});
    }

    template <typename ApplyFn>
    inline auto PropertyUndoable(EditorContext& context, char const* label, int& value, char const* action, ApplyFn&& applyFn,
        float speed = 1.0f, float min = 0.0f, float max = 0.0f, char const* format = nullptr) -> bool
    {
        PropertyOptions options{};
        options.speed = speed;
        options.min = min;
        options.max = max;
        options.format = format;
        return detail::undoableValue(context, label, value, action, std::forward<ApplyFn>(applyFn), options);
    }

    template <typename ApplyFn>
    inline auto PropertyUndoable(EditorContext& context, char const* label, float& value, char const* action, ApplyFn&& applyFn,
        float speed = 0.1f, float min = 0.0f, float max = 0.0f, char const* format = nullptr) -> bool
    {
        PropertyOptions options{};
        options.speed = speed;
        options.min = min;
        options.max = max;
        options.format = format;
        return detail::undoableValue(context, label, value, action, std::forward<ApplyFn>(applyFn), options);
    }

    template <typename ApplyFn>
    inline auto PropertyUndoable(EditorContext& context, char const* label, float3& value, char const* action, ApplyFn&& applyFn,
        float speed = 0.1f, float min = 0.0f, float max = 0.0f, float reset = 0.0f, char const* format = nullptr) -> bool
    {
        PropertyOptions options{};
        options.speed = speed;
        options.min = min;
        options.max = max;
        options.reset = reset;
        options.format = format;
        return detail::undoableValue(context, label, value, action, std::forward<ApplyFn>(applyFn), options);
    }

    template <typename ApplyFn>
    inline auto PropertyUndoable(EditorContext& context, char const* label, std::string& buffer, char const* action, ApplyFn&& applyFn) -> bool
    {
        auto oldValue = buffer;
        if (!ui::Property(label, buffer))
        {
            return false;
        }

        auto newValue = buffer;
        context.commandStack.apply(
            action,
            oldValue,
            newValue,
            [applyFn = std::forward<ApplyFn>(applyFn)](std::string const& v) mutable { applyFn(v); }
        );
        return true;
    }

    template <typename ComponentType, typename ValueType>
    inline auto PropertyUndoable(EditorContext& context,
        char const* label,
        ComponentType& component,
        ValueType ComponentType::* memberPtr,
        char const* commandName,
        float speed = 1.0f,
        float min = 0.0f,
        float max = 0.0f,
        float reset = 0.0f,
        char const* format = nullptr) -> bool
    {
        if constexpr (std::is_same_v<ValueType, bool>)
        {
            return PropertyUndoable(context, label, component.*memberPtr, commandName,
                [&component, memberPtr](bool value) {
                    component.*memberPtr = value;
                    detail::markDirtyIfPresent(component);
                });
        }
        else if constexpr (std::is_same_v<ValueType, std::string>)
        {
            return PropertyUndoable(context, label, component.*memberPtr, commandName,
                [&component, memberPtr](std::string const& value) {
                    component.*memberPtr = value;
                    detail::markDirtyIfPresent(component);
                });
        }
        else if constexpr (std::is_same_v<ValueType, float3>)
        {
            return PropertyUndoable(context, label, component.*memberPtr, commandName,
                [&component, memberPtr](float3 const& value) {
                    component.*memberPtr = value;
                    detail::markDirtyIfPresent(component);
                },
                speed, min, max, reset, format);
        }
        else if constexpr (std::is_same_v<ValueType, float>)
        {
            return PropertyUndoable(context, label, component.*memberPtr, commandName,
                [&component, memberPtr](float value) {
                    component.*memberPtr = value;
                    detail::markDirtyIfPresent(component);
                },
                speed, min, max, format);
        }
        else if constexpr (std::is_same_v<ValueType, int>)
        {
            return PropertyUndoable(context, label, component.*memberPtr, commandName,
                [&component, memberPtr](int value) {
                    component.*memberPtr = value;
                    detail::markDirtyIfPresent(component);
                },
                speed, min, max, format);
        }
        else
        {
            static_assert(!std::is_same_v<ValueType, ValueType>, "PropertyUndoable: unsupported member type.");
            return false;
        }
    }
}
