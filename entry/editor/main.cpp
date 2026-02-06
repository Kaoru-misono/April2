#include <runtime/engine.hpp>
#include <editor/editor-app.hpp>

int main()
{
    april::EngineConfig config{};
    config.window.title = "April Editor";
    config.device.enableDebugLayer = true;
    config.device.type = april::graphics::Device::Type::Default;
    config.compositeSceneToOutput = false;
    config.assetRoot = "content";
    config.ddcRoot = "build/cache/DDC";

    april::Engine engine(config);

    april::editor::EditorApp editor{};
    editor.setOnExit([&engine]() {
        engine.stop();
    });
    auto uiConfig = april::editor::EditorUiConfig{};
    uiConfig.enableViewports = true;
    uiConfig.iniFilename = "imgui_editor.ini";
    editor.install(engine, uiConfig);

    return engine.run();
}
