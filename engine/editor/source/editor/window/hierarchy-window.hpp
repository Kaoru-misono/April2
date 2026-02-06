#pragma once

#include <editor/tool-window.hpp>

namespace april::editor
{
    class HierarchyWindow final : public ToolWindow
    {
    public:
        HierarchyWindow() = default;

        [[nodiscard]] auto title() const -> char const* override { return "Hierarchy"; }
        auto onUIRender(EditorContext& context) -> void override;
    };
}
