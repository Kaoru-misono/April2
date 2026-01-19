#include "element-logger.hpp"
#include <core/log/logger.hpp>
#include <imgui_internal.h> // For ImGuiTextBuffer if needed, mostly covered by imgui.h

namespace april::ui
{
    ElementLogger::ElementLogger(bool show)
        : m_showLog(show)
    {
    }

    ElementLogger::~ElementLogger()
    {
        // Detach sink if not already done?
        // Ideally should be done in onDetach or user code.
        // We can access global logger here?
        april::Log::getLogger()->removeSink(std::shared_ptr<ILogSink>(this, [](ILogSink*){})); // Safe shared_ptr from this? No.
        // We cannot remove ourselves safely if we are not managing the shared_ptr.
        // onAttach registers us, onDetach should unregister.
    }

    auto ElementLogger::onAttach(ImGuiLayer* pLayer) -> void
    {
        // Register as sink
        // We need to pass shared_ptr to addSink.
        // If ElementLogger is created as unique_ptr or shared_ptr, we need to handle this.
        // Usually Elements are managed by shared_ptr in the system.
        // Assuming 'this' is managed by shared_ptr elsewhere, we can use shared_from_this() if we inherit enable_shared_from_this.
        // IElement inherits Object which inherits enable_shared_from_this?
        // Let's check Object.
    }

    auto ElementLogger::onDetach() -> void
    {
        // Unregister sink
        // april::Log::getLogger()->removeSink(shared_from_this());
    }

    auto ElementLogger::onResize(graphics::CommandContext* pContext, float2 const& size) -> void {}
    auto ElementLogger::onPreRender() -> void {}
    auto ElementLogger::onRender(graphics::CommandContext* pContext) -> void {}
    auto ElementLogger::onFileDrop(std::filesystem::path const& filename) -> void {}

    auto ElementLogger::onUIMenu() -> void
    {
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Log", nullptr, &m_showLog);
            ImGui::EndMenu();
        }
    }

    auto ElementLogger::onUIRender() -> void
    {
        if (m_showLog)
        {
            draw("Log", &m_showLog);
        }
    }

    auto ElementLogger::log(LogContext const& context, LogConfig const& config, std::string_view message) -> void
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        int old_size = m_buf.size();
        m_buf.append(message.data());
        m_buf.append("\n"); // Ensure newline
        for (int new_size = m_buf.size(); old_size < new_size; old_size++)
        {
            if (m_buf[old_size] == '\n')
            {
                m_lineOffsets.push_back(old_size + 1);
                m_lineLevels.push_back(static_cast<int>(context.level));
            }
        }
    }

    auto ElementLogger::clear() -> void
    {
        m_buf.clear();
        m_lineOffsets.clear();
        m_lineLevels.clear();
        m_lineOffsets.push_back(0);
    }

    auto ElementLogger::draw(const char* title, bool* p_open) -> void
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &m_autoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear_log = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        m_filter.Draw("Filter", -100.0f);

        if (clear_log)
            clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear_log)
            clear();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = m_buf.begin();
        const char* buf_end = m_buf.end();
        
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_filter.IsActive())
        {
            for (int line_no = 0; line_no < m_lineOffsets.Size; line_no++)
            {
                const char* line_start = buf + (line_no == 0 ? 0 : m_lineOffsets[line_no - 1]);
                const char* line_end = (line_no < m_lineOffsets.Size) ? (buf + m_lineOffsets[line_no] - 1) : buf_end;
                if (m_filter.PassFilter(line_start, line_end))
                {
                    // Color based on level (stored in m_lineLevels)
                    ImVec4 color = ImVec4(1, 1, 1, 1);
                    if (line_no < m_lineLevels.Size) {
                        switch ((ELogLevel)m_lineLevels[line_no]) {
                            case ELogLevel::Trace: color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
                            case ELogLevel::Debug: color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f); break;
                            case ELogLevel::Info:  color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
                            case ELogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
                            case ELogLevel::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break;
                            case ELogLevel::Fatal: color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                        }
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(line_start, line_end);
                    ImGui::PopStyleColor();
                }
            }
        }
        else
        {
            ImGuiListClipper clipper;
            clipper.Begin(m_lineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + (line_no == 0 ? 0 : m_lineOffsets[line_no - 1]);
                    const char* line_end = (line_no < m_lineOffsets.Size) ? (buf + m_lineOffsets[line_no] - 1) : buf_end;
                    
                    ImVec4 color = ImVec4(1, 1, 1, 1);
                    if (line_no < m_lineLevels.Size) {
                        switch ((ELogLevel)m_lineLevels[line_no]) {
                            case ELogLevel::Trace: color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
                            case ELogLevel::Debug: color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f); break;
                            case ELogLevel::Info:  color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
                            case ELogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
                            case ELogLevel::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break;
                            case ELogLevel::Fatal: color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                        }
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(line_start, line_end);
                    ImGui::PopStyleColor();
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
}
