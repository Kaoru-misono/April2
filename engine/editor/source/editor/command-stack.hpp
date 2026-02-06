#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace april::editor
{
    struct CommandEntry
    {
        std::string label{};
        std::function<void()> apply{};
        std::function<void()> undo{};
    };

    class CommandStack
    {
    public:
        auto execute(CommandEntry entry) -> void
        {
            if (!entry.apply)
            {
                return;
            }

            entry.apply();

            if (m_cursor < m_entries.size())
            {
                m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(m_cursor), m_entries.end());
            }

            m_entries.emplace_back(std::move(entry));
            m_cursor = m_entries.size();
        }

        auto execute(std::string label, std::function<void()> apply, std::function<void()> undo) -> void
        {
            execute(CommandEntry{std::move(label), std::move(apply), std::move(undo)});
        }

        auto undo() -> void
        {
            if (m_cursor == 0 || m_entries.empty())
            {
                return;
            }

            m_cursor -= 1;
            auto& entry = m_entries[m_cursor];
            if (entry.undo)
            {
                entry.undo();
            }
        }

        auto redo() -> void
        {
            if (m_cursor >= m_entries.size())
            {
                return;
            }

            auto& entry = m_entries[m_cursor];
            if (entry.apply)
            {
                entry.apply();
            }

            m_cursor += 1;
        }

        auto clear() -> void
        {
            m_entries.clear();
            m_cursor = 0;
        }

        [[nodiscard]] auto canUndo() const -> bool { return m_cursor > 0; }
        [[nodiscard]] auto canRedo() const -> bool { return m_cursor < m_entries.size(); }

        template <typename T, typename Setter>
        auto apply(std::string label, T const& oldValue, T const& newValue, Setter&& setter) -> void
        {
            if (oldValue == newValue)
            {
                return;
            }

            execute(
                std::move(label),
                [setter, newValue]() mutable { setter(newValue); },
                [setter, oldValue]() mutable { setter(oldValue); }
            );
        }

    private:
        std::vector<CommandEntry> m_entries{};
        size_t m_cursor{0};
    };
}
