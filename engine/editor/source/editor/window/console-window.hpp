#pragma once

#include <editor/tool-window.hpp>
#include <core/log/log-sink.hpp>

#include <imgui.h>

#include <mutex>
#include <memory>

namespace april::editor
{
    class ConsoleWindow final : public ToolWindow
    {
    public:
        explicit ConsoleWindow(bool show = false);
        ~ConsoleWindow() override;

        [[nodiscard]] auto title() const -> char const* override { return "Console"; }
        auto onUIRender(EditorContext& context) -> void override;

    private:
        class ConsoleSink final : public ILogSink
        {
            auto log(LogContext const& context, LogConfig const& config, std::string_view message) -> void override;

            friend class ConsoleWindow;

            ImGuiTextBuffer m_buf;
            ImVector<int> m_lineOffsets;
            ImVector<int> m_lineLevels;
            mutable std::mutex m_mutex;
        };

        auto draw() -> void;
        auto clear() -> void;

        ImGuiTextFilter m_filter;
        bool m_autoScroll = true;
        std::shared_ptr<ConsoleSink> m_sink{};
        bool m_registered = false;
    };
}
