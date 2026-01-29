#include "element-logger.hpp"
#include <core/log/logger.hpp>
#include <imgui_internal.h> // For ImGuiTextBuffer if needed, mostly covered by imgui.h

namespace april::ui
{
    ElementLogger::ElementLogger(bool show)
        : m_showLog(show)
    {
        m_sink = std::make_shared<ElementSink>();
    }

    ElementLogger::~ElementLogger()
    {
        if (m_registered && m_sink)
        {
            april::Log::getLogger()->removeSink(m_sink);
            m_registered = false;
        }
    }

    auto ElementLogger::onAttach(ImGuiLayer* pLayer) -> void
    {
        if (!m_registered && m_sink)
        {
            april::Log::getLogger()->addSink(m_sink);
            m_registered = true;
        }
    }

    auto ElementLogger::onDetach() -> void
    {
        if (m_registered && m_sink)
        {
            april::Log::getLogger()->removeSink(m_sink);
            m_registered = false;
        }
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

    auto ElementLogger::ElementSink::log(LogContext const& context, LogConfig const& config, std::string_view message) -> void
    {
        auto lock = std::lock_guard<std::mutex>{m_mutex};
        auto old_size = m_buf.size();
        m_buf.append(message.data());
        m_buf.append("\n"); // Ensure newline
        for (auto new_size = m_buf.size(); old_size < new_size; old_size++)
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
        if (!m_sink)
        {
            return;
        }
        auto lock = std::lock_guard<std::mutex>{m_sink->m_mutex};
        m_sink->m_buf.clear();
        m_sink->m_lineOffsets.clear();
        m_sink->m_lineLevels.clear();
        m_sink->m_lineOffsets.push_back(0);
    }

    auto ElementLogger::draw(char const* title, bool* p_open) -> void
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
        auto clear_log = ImGui::Button("Clear");
        ImGui::SameLine();
        auto copy = ImGui::Button("Copy");
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
        if (!m_sink)
        {
            ImGui::EndChild();
            ImGui::End();
            return;
        }

        auto lock = std::lock_guard<std::mutex>{m_sink->m_mutex};
        auto& buffer = m_sink->m_buf;
        auto* buf = buffer.begin();
        auto* buf_end = buffer.end();
        auto& lineOffsets = m_sink->m_lineOffsets;
        auto& lineLevels = m_sink->m_lineLevels;

        // std::lock_guard<std::mutex> lock(m_mutex);

        if (m_filter.IsActive())
        {
            for (auto line_no = 0; line_no < lineOffsets.Size; line_no++)
            {
                auto* line_start = buf + (line_no == 0 ? 0 : lineOffsets[line_no - 1]);
                auto* line_end = (line_no < lineOffsets.Size) ? (buf + lineOffsets[line_no] - 1) : buf_end;
                if (m_filter.PassFilter(line_start, line_end))
                {
                    // Color based on level (stored in m_lineLevels)
                    auto color = ImVec4(1, 1, 1, 1);
                    if (line_no < lineLevels.Size) {
                        switch ((ELogLevel)lineLevels[line_no]) {
                            case ELogLevel::Trace: color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
                            case ELogLevel::Debug: color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f); break;
                            case ELogLevel::Info:  color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
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
            auto clipper = ImGuiListClipper{};
            clipper.Begin(lineOffsets.Size);
            while (clipper.Step())
            {
                for (auto line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    auto* line_start = buf + (line_no == 0 ? 0 : lineOffsets[line_no - 1]);
                    auto* line_end = (line_no < lineOffsets.Size) ? (buf + lineOffsets[line_no] - 1) : buf_end;

                    auto color = ImVec4(1, 1, 1, 1);
                    if (line_no < lineLevels.Size) {
                        switch ((ELogLevel)lineLevels[line_no]) {
                            case ELogLevel::Trace: color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
                            case ELogLevel::Debug: color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f); break;
                            case ELogLevel::Info:  color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
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
