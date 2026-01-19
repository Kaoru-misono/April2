#pragma once

#include <ui/element.hpp>
#include <core/log/log-sink.hpp>
#include <imgui.h>
#include <mutex>
#include <vector>
#include <string>

namespace april::ui
{
    class ElementLogger : public IElement
    {
        APRIL_OBJECT(ElementLogger)

    public:
        ElementLogger(bool show = false);
        virtual ~ElementLogger();

        // IElement overrides
        auto onAttach(ImGuiLayer* pLayer) -> void override;
        auto onDetach() -> void override;
        auto onResize(graphics::CommandContext* pContext, float2 const& size) -> void override;
        auto onUIRender() -> void override;
        auto onUIMenu() -> void override;
        auto onPreRender() -> void override;
        auto onRender(graphics::CommandContext* pContext) -> void override;
        auto onFileDrop(std::filesystem::path const& filename) -> void override;

        // ILogSink overrides
        class ElementSink final: public ILogSink
        {
            auto log(LogContext const& context, LogConfig const& config, std::string_view message) -> void override;

            friend class ElementLogger;

            // TODO: move these to element.
            ImGuiTextBuffer m_buf;
            ImVector<int>   m_lineOffsets; // Index to lines offset.
            ImVector<int>   m_lineLevels;  // Log level per line.
            mutable std::mutex m_mutex;
        };

    private:
        auto draw(char const* title, bool* p_open = nullptr) -> void;
        auto clear() -> void;

    private:
        ImGuiTextFilter m_filter;
        bool            m_autoScroll = true;
        bool            m_showLog = false;
        std::shared_ptr<ElementSink> m_sink{};
    };
}
