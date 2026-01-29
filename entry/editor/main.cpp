#include <runtime/engine.hpp>
#include <editor/editor-layer.hpp>

int main()
{
    april::EngineConfig config{};
    config.window.title = "April Editor";
    config.device.enableDebugLayer = true;
    config.device.type = april::graphics::Device::Type::Default;
    config.imgui.hasUndockableViewport = true;

    april::Engine engine(config);

    auto layer = april::core::make_ref<april::editor::EditorLayer>();
    layer->setOnExit([&engine]() {
        engine.stop();
    });
    engine.addElement(layer);

    return engine.run();
}
