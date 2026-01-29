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

        virtual auto onAttach(ImGuiLayer*) -> void {};                              // Called once at start
        virtual auto onDetach() -> void {};                                         // Called before destroying
        virtual auto onResize(graphics::CommandContext*, float2 const&) -> void {}; // Called when the viewport size is changing
        virtual auto onUIRender() -> void {}                                        // Called for anything related to UI
        virtual auto onUIMenu() -> void {}                                          // This is the menubar to create
        virtual auto onPreRender() -> void {}                                       // called post onUIRender and prior onRender (looped over all elements)
        virtual auto onRender(graphics::CommandContext*) -> void {}                 // For anything to render within a frame
        virtual auto onFileDrop(std::filesystem::path const&) -> void {}            // For when a file is dragged on top of the window

        virtual ~IElement() = default;
    };
}
