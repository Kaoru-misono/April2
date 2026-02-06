#pragma once

#include <graphics/rhi/command-context.hpp>
#include <core/foundation/object.hpp>
#include <core/math/type.hpp>

#include <filesystem>

namespace april::editor
{
    class ImGuiBackend;

    struct IEditorElement : public core::Object
    {
        APRIL_OBJECT(IEditorElement)

        virtual auto onAttach(ImGuiBackend*) -> void {}
        virtual auto onDetach() -> void {}
        virtual auto onResize(graphics::CommandContext*, float2 const&) -> void {}
        virtual auto onUIRender() -> void {}
        virtual auto onUIMenu() -> void {}
        virtual auto onPreRender() -> void {}
        virtual auto onRender(graphics::CommandContext*) -> void {}
        virtual auto onFileDrop(std::filesystem::path const&) -> void {}

        virtual ~IEditorElement() = default;
    };
}
