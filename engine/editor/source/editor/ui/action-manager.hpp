#pragma once

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace april::editor::ui
{
    struct ShortcutBinding
    {
        ImGuiKey key{ImGuiKey_None};
        bool ctrl{false};
        bool shift{false};
        bool alt{false};
        bool super{false};
        bool valid{false};
    };

    struct EditorAction
    {
        std::string menu{};
        std::string name{};
        std::string shortcut{};
        std::function<void()> callback{};
        std::function<bool()> isEnabled{};
        std::function<bool()> isChecked{};
        ShortcutBinding binding{};
    };

    class ActionManager
    {
    public:
        auto registerAction(
            std::string menu,
            std::string name,
            std::string shortcut,
            std::function<void()> callback,
            std::function<bool()> isEnabled = {},
            std::function<bool()> isChecked = {}
        ) -> EditorAction*
        {
            EditorAction action{};
            action.menu = std::move(menu);
            action.name = std::move(name);
            action.shortcut = std::move(shortcut);
            action.callback = std::move(callback);
            action.isEnabled = std::move(isEnabled);
            action.isChecked = std::move(isChecked);
            action.binding = parseShortcut(action.shortcut);

            auto const index = m_actions.size();
            m_actionLookup[action.name] = index;
            m_menuLookup[action.menu].push_back(index);
            m_actions.emplace_back(std::move(action));
            return &m_actions.back();
        }

        auto getAction(std::string_view name) -> EditorAction*
        {
            auto it = m_actionLookup.find(std::string{name});
            if (it == m_actionLookup.end())
            {
                return nullptr;
            }
            return &m_actions[it->second];
        }

        auto getMenuActions(std::string_view menu) -> std::vector<EditorAction*>
        {
            std::vector<EditorAction*> result{};
            auto it = m_menuLookup.find(std::string{menu});
            if (it == m_menuLookup.end())
            {
                return result;
            }

            result.reserve(it->second.size());
            for (auto index : it->second)
            {
                result.push_back(&m_actions[index]);
            }
            return result;
        }

        auto trigger(std::string_view name) -> bool
        {
            auto* action = getAction(name);
            if (!action || !action->callback)
            {
                return false;
            }
            if (action->isEnabled && !action->isEnabled())
            {
                return false;
            }
            action->callback();
            return true;
        }

        auto processShortcuts(bool respectTextInput = true) -> void
        {
            auto const& io = ImGui::GetIO();
            if (respectTextInput && io.WantCaptureKeyboard)
            {
                return;
            }

            for (auto& action : m_actions)
            {
                if (!action.binding.valid || !action.callback)
                {
                    continue;
                }
                if (action.isEnabled && !action.isEnabled())
                {
                    continue;
                }
                if (shortcutActive(action.binding))
                {
                    action.callback();
                }
            }
        }

    private:
        static auto trim(std::string_view value) -> std::string_view
        {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
            {
                value.remove_prefix(1);
            }
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
            {
                value.remove_suffix(1);
            }
            return value;
        }

        static auto toUpper(std::string_view value) -> std::string
        {
            std::string upper{};
            upper.reserve(value.size());
            for (auto ch : value)
            {
                upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            }
            return upper;
        }

        static auto parseKeyToken(std::string_view token) -> ImGuiKey
        {
            if (token.empty())
            {
                return ImGuiKey_None;
            }

            if (token.size() == 1)
            {
                auto c = token.front();
                if (c >= 'A' && c <= 'Z')
                {
                    return static_cast<ImGuiKey>(ImGuiKey_A + (c - 'A'));
                }
                if (c >= '0' && c <= '9')
                {
                    return static_cast<ImGuiKey>(ImGuiKey_0 + (c - '0'));
                }
            }

            if (token[0] == 'F' && token.size() <= 3)
            {
                auto number = std::atoi(std::string{token.substr(1)}.c_str());
                if (number >= 1 && number <= 12)
                {
                    return static_cast<ImGuiKey>(ImGuiKey_F1 + (number - 1));
                }
            }

            if (token == "TAB") return ImGuiKey_Tab;
            if (token == "ESC" || token == "ESCAPE") return ImGuiKey_Escape;
            if (token == "ENTER" || token == "RETURN") return ImGuiKey_Enter;
            if (token == "SPACE") return ImGuiKey_Space;
            if (token == "BACKSPACE") return ImGuiKey_Backspace;
            if (token == "DELETE" || token == "DEL") return ImGuiKey_Delete;

            return ImGuiKey_None;
        }

        static auto shortcutActive(ShortcutBinding const& binding) -> bool
        {
            auto const& io = ImGui::GetIO();
            if (binding.ctrl != io.KeyCtrl) return false;
            if (binding.shift != io.KeyShift) return false;
            if (binding.alt != io.KeyAlt) return false;
            if (binding.super != io.KeySuper) return false;
            return ImGui::IsKeyPressed(binding.key, false);
        }

        auto parseShortcut(std::string_view shortcut) const -> ShortcutBinding
        {
            ShortcutBinding binding{};
            auto remaining = shortcut;

            while (!remaining.empty())
            {
                auto plusPos = remaining.find('+');
                auto token = plusPos == std::string_view::npos ? remaining : remaining.substr(0, plusPos);
                remaining = plusPos == std::string_view::npos ? std::string_view{} : remaining.substr(plusPos + 1);

                token = trim(token);
                if (token.empty())
                {
                    continue;
                }

                auto upper = toUpper(token);
                if (upper == "CTRL" || upper == "CONTROL")
                {
                    binding.ctrl = true;
                    continue;
                }
                if (upper == "SHIFT")
                {
                    binding.shift = true;
                    continue;
                }
                if (upper == "ALT" || upper == "OPTION")
                {
                    binding.alt = true;
                    continue;
                }
                if (upper == "CMD" || upper == "SUPER" || upper == "WIN" || upper == "META")
                {
                    binding.super = true;
                    continue;
                }

                binding.key = parseKeyToken(upper);
            }

            binding.valid = binding.key != ImGuiKey_None;
            return binding;
        }

        std::vector<EditorAction> m_actions{};
        std::unordered_map<std::string, size_t> m_actionLookup{};
        std::unordered_map<std::string, std::vector<size_t>> m_menuLookup{};
    };
}
