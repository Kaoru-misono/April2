#include <runtime/engine.hpp>
#include <editor/editor-app.hpp>

int main()
{
    april::EngineConfig config{};
    config.window.title = "April Editor";
    config.device.enableDebugLayer = true;
    config.device.type = april::graphics::Device::Type::Default;
    config.imgui.hasUndockableViewport = true;
    config.compositeSceneToOutput = false;
    config.imguiIniFilename = "imgui_editor.ini";
    config.assetRoot = "content";
    config.ddcRoot = "build/cache/DDC";

    april::Engine engine(config);

    april::editor::EditorApp editor{};
    editor.setOnExit([&engine]() {
        engine.stop();
    });
    editor.install(engine);

    return engine.run();
}
