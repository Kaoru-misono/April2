#pragma once

#include <graphics/rhi/command-context.hpp>
#include <core/foundation/object.hpp>

namespace april::ui
{
    class ImGuiLayer;

    struct IElement : public core::Object
    {
        APRIL_OBJECT(IElement)
        virtual void onAttach(ImGuiLayer* pLayer) = 0;                       // Called once at start
        virtual void onDetach() = 0;                                         // Called before destroying
        virtual void onResize(graphics::CommandContext* pContext, float2 const& size) = 0; // Called when the viewport size is changing
        virtual void onUIRender() = 0;                                           // Called for anything related to UI
        virtual void onUIMenu() = 0;                                             // This is the menubar to create
        virtual void onPreRender() = 0;                  // called post onUIRender and prior onRender (looped over all elements)
        virtual void onRender(graphics::CommandContext* pContext) = 0;  // For anything to render within a frame
        virtual void onFileDrop(std::filesystem::path const& filename) = 0;  // For when a file is dragged on top of the window

        virtual ~IElement() = default;
    };
}
