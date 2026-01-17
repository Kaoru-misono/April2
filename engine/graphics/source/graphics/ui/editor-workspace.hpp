#pragma once

#include <core/foundation/object.hpp>
#include <core/log/logger.hpp>
#include <graphics/rhi/fwd.hpp>
#include "editor-panel.hpp"
#include <vector>
#include <memory>
#include <filesystem>
#include <mutex>

namespace april::graphics
{
    /**
     * @brief Manages the main ImGui dockspace and editor panels.
     */
    class EditorWorkspace final : public core::Object
    {
        APRIL_OBJECT(EditorWorkspace)
    public:
        EditorWorkspace();
        ~EditorWorkspace() override;

        auto init() -> void;
        auto addPanel(core::ref<EditorPanel> pPanel) -> void;
        auto onUIRender() -> void;

        auto setViewportTexture(core::ref<ShaderResourceView> pSRV) -> void { mp_viewportSRV = pSRV; }

    private:
        auto setupDockspace() -> void;
        auto renderContentBrowser() -> void;
        auto renderConsole() -> void;
        auto renderViewport() -> void;
        auto renderHierarchy() -> void;
        auto renderInspector() -> void;

    private:
        std::vector<core::ref<EditorPanel>> m_panels;
        uint32_t m_dockID{0};
        bool m_firstFrame{true};

        // Viewport State
        core::ref<ShaderResourceView> mp_viewportSRV;

        // Content Browser State
        std::filesystem::path m_currentContentPath{"."};

        // Console State
        struct LogMessage
        {
            ELogLevel level;
            std::string prefix;
            std::string message;
        };
        std::vector<LogMessage> m_logMessages;
        Logger::LogSinkId m_logSinkId{0};
    };
}
