#pragma once

#include <imgui.h>

#include <utility>

namespace april::editor::ui
{
    class ScopedWindow
    {
    public:
        ScopedWindow(char const* title, bool* open = nullptr, ImGuiWindowFlags flags = 0)
        {
            m_visible = ImGui::Begin(title, open, flags);
            m_active = true;
        }

        ~ScopedWindow()
        {
            if (m_active)
            {
                ImGui::End();
            }
        }

        ScopedWindow(ScopedWindow const&) = delete;
        ScopedWindow& operator=(ScopedWindow const&) = delete;

        ScopedWindow(ScopedWindow&& other) noexcept
            : m_visible(other.m_visible), m_active(std::exchange(other.m_active, false))
        {
        }

        [[nodiscard]] explicit operator bool() const { return m_visible; }

    private:
        bool m_visible{false};
        bool m_active{false};
    };

    class ScopedMenu
    {
    public:
        ScopedMenu(char const* label, bool enabled = true)
        {
            m_open = ImGui::BeginMenu(label, enabled);
        }

        ~ScopedMenu()
        {
            if (m_open)
            {
                ImGui::EndMenu();
            }
        }

        ScopedMenu(ScopedMenu const&) = delete;
        ScopedMenu& operator=(ScopedMenu const&) = delete;

        ScopedMenu(ScopedMenu&& other) noexcept
            : m_open(std::exchange(other.m_open, false))
        {
        }

        [[nodiscard]] explicit operator bool() const { return m_open; }

    private:
        bool m_open{false};
    };

    class ScopedMainMenuBar
    {
    public:
        ScopedMainMenuBar()
        {
            m_open = ImGui::BeginMainMenuBar();
        }

        ~ScopedMainMenuBar()
        {
            if (m_open)
            {
                ImGui::EndMainMenuBar();
            }
        }

        ScopedMainMenuBar(ScopedMainMenuBar const&) = delete;
        ScopedMainMenuBar& operator=(ScopedMainMenuBar const&) = delete;

        ScopedMainMenuBar(ScopedMainMenuBar&& other) noexcept
            : m_open(std::exchange(other.m_open, false))
        {
        }

        [[nodiscard]] explicit operator bool() const { return m_open; }

    private:
        bool m_open{false};
    };

    class ScopedStyle
    {
    public:
        template <typename T, typename... Rest>
        explicit ScopedStyle(ImGuiStyleVar var, T&& value, Rest&&... rest)
        {
            static_assert(sizeof...(Rest) % 2 == 0, "ScopedStyle expects (ImGuiStyleVar, value) pairs.");
            ImGui::PushStyleVar(var, std::forward<T>(value));
            m_count = 1 + pushRest(std::forward<Rest>(rest)...);
        }

        ~ScopedStyle()
        {
            if (m_count > 0)
            {
                ImGui::PopStyleVar(m_count);
            }
        }

        ScopedStyle(ScopedStyle const&) = delete;
        ScopedStyle& operator=(ScopedStyle const&) = delete;

        ScopedStyle(ScopedStyle&& other) noexcept
            : m_count(std::exchange(other.m_count, 0))
        {
        }

    private:
        static int pushRest() { return 0; }

        template <typename T, typename... Rest>
        static int pushRest(ImGuiStyleVar var, T&& value, Rest&&... rest)
        {
            ImGui::PushStyleVar(var, std::forward<T>(value));
            return 1 + pushRest(std::forward<Rest>(rest)...);
        }

        int m_count{0};
    };

    class ScopedStyleColor
    {
    public:
        template <typename T, typename... Rest>
        explicit ScopedStyleColor(ImGuiCol col, T&& color, Rest&&... rest)
        {
            static_assert(sizeof...(Rest) % 2 == 0, "ScopedStyleColor expects (ImGuiCol, color) pairs.");
            ImGui::PushStyleColor(col, std::forward<T>(color));
            m_count = 1 + pushRest(std::forward<Rest>(rest)...);
        }

        ~ScopedStyleColor()
        {
            if (m_count > 0)
            {
                ImGui::PopStyleColor(m_count);
            }
        }

        ScopedStyleColor(ScopedStyleColor const&) = delete;
        ScopedStyleColor& operator=(ScopedStyleColor const&) = delete;

        ScopedStyleColor(ScopedStyleColor&& other) noexcept
            : m_count(std::exchange(other.m_count, 0))
        {
        }

    private:
        static int pushRest() { return 0; }

        template <typename T, typename... Rest>
        static int pushRest(ImGuiCol col, T&& color, Rest&&... rest)
        {
            ImGui::PushStyleColor(col, std::forward<T>(color));
            return 1 + pushRest(std::forward<Rest>(rest)...);
        }

        int m_count{0};
    };

    class ScopedID
    {
    public:
        explicit ScopedID(int id) { ImGui::PushID(id); }
        explicit ScopedID(char const* id) { ImGui::PushID(id); }
        explicit ScopedID(void const* id) { ImGui::PushID(id); }

        ~ScopedID()
        {
            if (m_active)
            {
                ImGui::PopID();
            }
        }

        ScopedID(ScopedID const&) = delete;
        ScopedID& operator=(ScopedID const&) = delete;

        ScopedID(ScopedID&& other) noexcept
            : m_active(std::exchange(other.m_active, false))
        {
        }

    private:
        bool m_active{true};
    };

    class ScopedChild
    {
    public:
        explicit ScopedChild(char const* id, ImVec2 size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0)
        {
            m_open = ImGui::BeginChild(id, size, border, flags);
        }

        ~ScopedChild()
        {
            ImGui::EndChild();
        }

        ScopedChild(ScopedChild const&) = delete;
        ScopedChild& operator=(ScopedChild const&) = delete;

        ScopedChild(ScopedChild&& other) noexcept
            : m_open(std::exchange(other.m_open, false))
        {
        }

        [[nodiscard]] explicit operator bool() const { return m_open; }

    private:
        bool m_open{false};
    };

    class ScopedTable
    {
    public:
        explicit ScopedTable(char const* id, int columns, ImGuiTableFlags flags = 0, ImVec2 size = ImVec2(0, 0), float innerWidth = 0.0f)
        {
            m_open = ImGui::BeginTable(id, columns, flags, size, innerWidth);
        }

        ~ScopedTable()
        {
            if (m_open)
            {
                ImGui::EndTable();
            }
        }

        ScopedTable(ScopedTable const&) = delete;
        ScopedTable& operator=(ScopedTable const&) = delete;

        ScopedTable(ScopedTable&& other) noexcept
            : m_open(std::exchange(other.m_open, false))
        {
        }

        [[nodiscard]] explicit operator bool() const { return m_open; }

    private:
        bool m_open{false};
    };
}
