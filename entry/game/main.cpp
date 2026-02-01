#include <runtime/engine.hpp>
#include <imgui.h>

int main()
{
    april::EngineConfig config{};
    config.window.title = "April Game";
    config.device.enableDebugLayer = true;
    config.device.type = april::graphics::Device::Type::Vulkan;
    config.imgui.useMenu = false;
    config.imgui.imguiConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
    config.imguiIniFilename = "imgui_game.ini";

    april::Engine engine(config);
    return engine.run();
}
