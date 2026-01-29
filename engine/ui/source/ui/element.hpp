#pragma once

#include <graphics/rhi/command-context.hpp>
#include <core/foundation/object.hpp>

#include <filesystem>

namespace april::ui
{
    class ImGuiLayer;

    struct IElement : public core::Object
    {
        APRIL_OBJECT(IElement)

        virtual auto onAttach(ImGuiLayer* pLayer) -> void = 0;                                     // Called once at start
        virtual auto onDetach() -> void = 0;                                                       // Called before destroying
        virtual auto onResize(graphics::CommandContext* pContext, float2 const& size) -> void = 0; // Called when the viewport size is changing
        virtual auto onUIRender() -> void = 0;                                                     // Called for anything related to UI
        virtual auto onUIMenu() -> void = 0;                                                       // This is the menubar to create
        virtual auto onPreRender() -> void = 0;                                                    // called post onUIRender and prior onRender (looped over all elements)
        virtual auto onRender(graphics::CommandContext* pContext) -> void = 0;                     // For anything to render within a frame
        virtual auto onFileDrop(std::filesystem::path const& filename) -> void = 0;                // For when a file is dragged on top of the window

        virtual ~IElement() = default;
    };
}
