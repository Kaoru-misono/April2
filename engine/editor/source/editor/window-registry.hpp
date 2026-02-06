#pragma once

#include "tool-window.hpp"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace april::editor
{
    class WindowRegistry
    {
    public:
        template <typename T, typename... Args>
        auto emplace(Args&&... args) -> T&
        {
            auto window = std::make_unique<T>(std::forward<Args>(args)...);
            auto* ptr = window.get();
            m_windows.emplace_back(std::move(window));
            return *ptr;
        }

        auto add(std::unique_ptr<ToolWindow> window) -> void
        {
            if (window)
            {
                m_windows.emplace_back(std::move(window));
            }
        }

        [[nodiscard]] auto windows() -> std::vector<std::unique_ptr<ToolWindow>>& { return m_windows; }
        [[nodiscard]] auto windows() const -> std::vector<std::unique_ptr<ToolWindow>> const& { return m_windows; }

        [[nodiscard]] auto findByTitle(std::string_view title) -> ToolWindow*
        {
            for (auto& window : m_windows)
            {
                if (window && window->title() && title == window->title())
                {
                    return window.get();
                }
            }
            return nullptr;
        }

    private:
        std::vector<std::unique_ptr<ToolWindow>> m_windows{};
    };
}
