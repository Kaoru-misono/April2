#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>
#include <glm/gtc/constants.hpp>

int main()
{
    april::EngineConfig config{};
    config.window.title = "April Game - Scene ECS Test";
    config.device.enableDebugLayer = true;
    config.device.type = april::graphics::Device::Type::D3D12;
    config.enableUI = false;  // Disable UI for testing
    config.compositeSceneToOutput = true;  // Disable compositing (no blit shader)
    config.imgui.useMenu = false;
    config.imgui.imguiConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
    config.imguiIniFilename = "imgui_game.ini";

    april::EngineHooks hooks{};
    hooks.onInit = []() {
        auto& engine = april::Engine::get();
        auto* scene = engine.getSceneGraph();
        if (!scene)
        {
            return;
        }

        auto& registry = scene->getRegistry();

        // Create main camera
        auto camera = scene->createEntity("MainCamera");
        auto& camComponent = registry.emplace<april::scene::CameraComponent>(camera);
        camComponent.isPerspective = true;
        camComponent.fov = glm::radians(45.0f);
        camComponent.nearClip = 0.1f;
        camComponent.farClip = 1000.0f;

        auto& camTransform = registry.get<april::scene::TransformComponent>(camera);
        camTransform.localPosition = {0.0f, 3.0f, 10.0f};
        camTransform.isDirty = true;

        // Create a cube entity
        auto cube = scene->createEntity("Cube");
        auto& meshRenderer = registry.emplace<april::scene::MeshRendererComponent>(cube);
        meshRenderer.meshAssetPath = "E:/github/April2/content/model/cube.gltf.asset";
        meshRenderer.enabled = true;

        // Optional: rotate the cube
        auto& cubeTransform = registry.get<april::scene::TransformComponent>(cube);
        cubeTransform.localRotation.y = glm::radians(45.0f);
        cubeTransform.isDirty = true;
    };

    april::Engine engine(config, hooks);
    return engine.run();
}
