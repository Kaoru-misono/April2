#pragma once

namespace april::editor
{
    struct EditorContext;

    class ToolWindow
    {
    public:
        virtual ~ToolWindow() = default;

        [[nodiscard]] virtual auto title() const -> char const* = 0;
        virtual auto onUIRender(EditorContext& context) -> void = 0;

        bool open = true;
    };
}
