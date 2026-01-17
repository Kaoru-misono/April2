#include "imgui-layer.hpp"
#include "slang-rhi-imgui-backend.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <core/window/window.hpp>
#include <core/math/type.hpp>
#include <core/log/logger.hpp>
#include <core/error/assert.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

namespace april::graphics
{
    ImGuiLayer::ImGuiLayer(Window& window, core::ref<graphics::Device> pDevice)
        : m_window{window}
        , mp_device{pDevice}
        , m_editorMode{true}
    {
        initApi();

        float xscale, yscale;
        glfwGetWindowContentScale((GLFWwindow*)m_window.getBackendWindow(), &xscale, &yscale);

        mp_backend = std::make_unique<SlangRHIImGuiBackend>();
        mp_backend->init(mp_device, xscale);
        mp_workspace = std::make_unique<EditorWorkspace>();
        mp_workspace->init();
        
        m_lastDpiScale = xscale;
        m_lastFinalScale = xscale;
    }

    ImGuiLayer::~ImGuiLayer()
    {
        if (mp_backend)
        {
            mp_backend->shutdown();
        }
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    auto ImGuiLayer::initApi() -> void
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsClassic();
        m_baseStyle = ImGui::GetStyle();

        auto* glfwWindow = static_cast<GLFWwindow*>(m_window.getBackendWindow());
        if (mp_device->getType() == graphics::Device::Type::Vulkan)
        {
            ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);
        }
        else
        {
            ImGui_ImplGlfw_InitForOther(glfwWindow, true);
        }
    }

    auto ImGuiLayer::setViewportTexture(core::ref<ShaderResourceView> pSRV) -> void
    {
        if (mp_workspace)
        {
            mp_workspace->setViewportTexture(pSRV);
        }
    }

    auto ImGuiLayer::begin() -> void
    {
        auto& io = ImGui::GetIO();
        float xscale, yscale;
        glfwGetWindowContentScale((GLFWwindow*)m_window.getBackendWindow(), &xscale, &yscale);
        
        float currentFinalScale = xscale * m_fontScale;

        // Check if we need a sharp rebuild
        if (std::abs(currentFinalScale - m_lastFinalScale) > 0.001f && m_fontDirty)
        {
            mp_device->wait(); // Wait for GPU
            
            mp_backend->shutdown();
            io.Fonts->Clear();
            mp_backend->init(mp_device, currentFinalScale);
            
            ImGui::GetStyle() = m_baseStyle;
            ImGui::GetStyle().ScaleAllSizes(currentFinalScale);
            
            io.FontGlobalScale = 1.0f; // Reset to sharp 1.0
            m_lastFinalScale = currentFinalScale;
            m_lastDpiScale = xscale;
            m_fontDirty = false;
        }
        // Handle DPI change automatically
        else if (std::abs(xscale - m_lastDpiScale) > 0.001f)
        {
            mp_device->wait();
            mp_backend->shutdown();
            io.Fonts->Clear();
            mp_backend->init(mp_device, currentFinalScale);
            
            ImGui::GetStyle() = m_baseStyle;
            ImGui::GetStyle().ScaleAllSizes(currentFinalScale);
            
            io.FontGlobalScale = 1.0f;
            m_lastFinalScale = currentFinalScale;
            m_lastDpiScale = xscale;
        }

        ImGui_ImplGlfw_NewFrame();

        if (mp_backend)
        {
            mp_backend->newFrame();
        }

        ImGui::NewFrame();

        if (m_editorMode && mp_workspace)
        {
            mp_workspace->onUIRender();
        }
    }

    auto ImGuiLayer::end(graphics::CommandContext* pCtx, graphics::RenderTargetView* pTargetView) -> void
    {
        ImGui::Render();

        auto* drawData = ImGui::GetDrawData();

        if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) return;

        graphics::ColorTargets colorTargets = {
            graphics::ColorTarget(core::ref<graphics::RenderTargetView>(pTargetView), graphics::LoadOp::Load, graphics::StoreOp::Store)
        };

        auto pEncoder = pCtx->beginRenderPass(colorTargets);
        if (mp_backend)
        {
            mp_backend->renderDrawData(drawData, pEncoder);
        }
        pEncoder->end();
    }
}
