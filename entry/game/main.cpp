#include <runtime/engine.hpp>
#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <core/input/input.hpp>
#include <core/log/logger.hpp>
#include <core/math/type.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

struct TestSceneState
{
    april::scene::Entity cameraMain{april::scene::NullEntity};
    april::scene::Entity cameraAlt{april::scene::NullEntity};
    april::scene::Entity cubeRoot{april::scene::NullEntity};
    april::scene::Entity cubeChild{april::scene::NullEntity};
    april::scene::Entity activeCamera{april::scene::NullEntity};
    float elapsed{0.0f};
    bool switched{false};
    float yaw{0.0f};
    float pitch{0.0f};
    float lastLogTime{0.0f};
    float orbitAngle{0.0f};
};

int main()
{
    april::EngineConfig config{};
    config.window.title = "April Game - Scene ECS Test";
    config.device.enableDebugLayer = true;
    config.device.type = april::graphics::Device::Type::D3D12;
    config.compositeSceneToOutput = true;  // Composite scene to output

    auto testState = TestSceneState{};

    april::EngineHooks hooks{};
    hooks.onInit = [&testState]() {
        auto& engine = april::Engine::get();
        auto* scene = engine.getSceneGraph();
        if (!scene)
        {
            return;
        }

        auto& registry = scene->getRegistry();

        // Create main camera
        auto camera = scene->createEntity("MainCamera");
        testState.cameraMain = camera;
        auto& camComponent = registry.emplace<april::scene::CameraComponent>(camera);
        camComponent.isPerspective = true;
        camComponent.fov = glm::radians(45.0f);
        camComponent.nearClip = 0.1f;
        camComponent.farClip = 1000.0f;

        auto& camTransform = registry.get<april::scene::TransformComponent>(camera);
        camTransform.localPosition = {0.0f, 3.0f, 10.0f};
        camTransform.isDirty = true;

        testState.activeCamera = camera;
        testState.yaw = camTransform.localRotation.y;
        testState.pitch = camTransform.localRotation.x;

        // Create a cube entity
        auto cube = scene->createEntity("Cube");
        testState.cubeRoot = cube;
        auto& meshRenderer = registry.emplace<april::scene::MeshRendererComponent>(cube);
        meshRenderer.meshAssetPath = "E:/github/April2/content/model/cube.gltf.asset";
        meshRenderer.enabled = true;

        // Create a second cube entity
        auto cube2 = scene->createEntity("Cube2");
        testState.cubeChild = cube2;
        auto& meshRenderer2 = registry.emplace<april::scene::MeshRendererComponent>(cube2);
        meshRenderer2.meshAssetPath = "E:/github/April2/content/model/cube.gltf.asset";
        meshRenderer2.enabled = true;

        auto& cube2Transform = registry.get<april::scene::TransformComponent>(cube2);
        cube2Transform.localPosition = {6.0f, 1.0f, 0.0f};
        cube2Transform.localScale = {0.6f, 0.6f, 0.6f};
        cube2Transform.isDirty = true;

        // Parent cube2 to cube and rotate parent
        scene->setParent(cube2, cube);

        auto& cubeTransform = registry.get<april::scene::TransformComponent>(cube);
        cubeTransform.localRotation.y = glm::radians(45.0f);
        cubeTransform.isDirty = true;

        // Create a second camera for switching test
        auto cameraAlt = scene->createEntity("AltCamera");
        testState.cameraAlt = cameraAlt;
        auto& camAltComponent = registry.emplace<april::scene::CameraComponent>(cameraAlt);
        camAltComponent.isPerspective = true;
        camAltComponent.fov = glm::radians(60.0f);
        camAltComponent.nearClip = 0.1f;
        camAltComponent.farClip = 1000.0f;

        auto& camAltTransform = registry.get<april::scene::TransformComponent>(cameraAlt);
        camAltTransform.localPosition = {8.0f, 5.0f, 12.0f};
        camAltTransform.localRotation.y = glm::radians(-35.0f);
        camAltTransform.localRotation.x = glm::radians(-10.0f);
        camAltTransform.isDirty = true;
    };

    hooks.onUpdate = [&testState](float delta) {
        testState.elapsed += delta;

        auto& engine = april::Engine::get();
        auto* scene = engine.getSceneGraph();
        if (!scene)
        {
            return;
        }

        auto& registry = scene->getRegistry();

        if (testState.elapsed - testState.lastLogTime >= 1.0f)
        {
            auto mouseDelta = april::Input::getMouseDelta();
            AP_INFO(
                "[GameInput] RMB={} W={} A={} S={} D={} Q={} E={} MouseDelta=({}, {})",
                april::Input::isMouseDown(april::MouseButton::Right),
                april::Input::isKeyDown(april::Key::W),
                april::Input::isKeyDown(april::Key::A),
                april::Input::isKeyDown(april::Key::S),
                april::Input::isKeyDown(april::Key::D),
                april::Input::isKeyDown(april::Key::Q),
                april::Input::isKeyDown(april::Key::E),
                mouseDelta.x,
                mouseDelta.y
            );
            testState.lastLogTime = testState.elapsed;
        }

        auto const activeCamera = scene->getActiveCamera();
        if (activeCamera == april::scene::NullEntity)
        {
            return;
        }

        if (activeCamera != testState.activeCamera)
        {
            testState.activeCamera = activeCamera;
            auto const& activeTransform = registry.get<april::scene::TransformComponent>(activeCamera);
            testState.yaw = activeTransform.localRotation.y;
            testState.pitch = activeTransform.localRotation.x;
        }

        auto& cameraTransform = registry.get<april::scene::TransformComponent>(activeCamera);

        bool const manualLook = april::Input::isMouseDown(april::MouseButton::Right) && april::Input::shouldProcessMouse();
        if (manualLook)
        {
            auto const mouseDelta = april::Input::getMouseDelta();
            testState.yaw += mouseDelta.x * 0.003f;
            testState.pitch += mouseDelta.y * 0.003f;
            testState.pitch = std::clamp(testState.pitch, -1.5f, 1.5f);

            cameraTransform.localRotation = {testState.pitch, testState.yaw, 0.0f};
            cameraTransform.isDirty = true;
        }

        auto moveRequested = false;
        if (april::Input::shouldProcessKeyboard())
        {
            auto rotation = glm::mat4{1.0f};
            rotation = glm::rotate(rotation, testState.yaw, april::float3{0.0f, 1.0f, 0.0f});
            rotation = glm::rotate(rotation, testState.pitch, april::float3{1.0f, 0.0f, 0.0f});

            auto const forward4 = rotation * april::float4{0.0f, 0.0f, -1.0f, 0.0f};
            auto forward = glm::normalize(april::float3{forward4.x, forward4.y, forward4.z});
            auto up = april::float3{0.0f, 1.0f, 0.0f};
            auto right = glm::normalize(glm::cross(forward, up));

            auto moveDir = april::float3{0.0f, 0.0f, 0.0f};
            if (april::Input::isKeyDown(april::Key::W)) moveDir += forward;
            if (april::Input::isKeyDown(april::Key::S)) moveDir -= forward;
            if (april::Input::isKeyDown(april::Key::A)) moveDir -= right;
            if (april::Input::isKeyDown(april::Key::D)) moveDir += right;
            if (april::Input::isKeyDown(april::Key::Q)) moveDir -= up;
            if (april::Input::isKeyDown(april::Key::E)) moveDir += up;

            if (moveDir.x != 0.0f || moveDir.y != 0.0f || moveDir.z != 0.0f)
            {
                moveRequested = true;
                auto speed = 5.0f;
                if (april::Input::isKeyDown(april::Key::LeftShift))
                {
                    speed *= 2.5f;
                }

                cameraTransform.localPosition += glm::normalize(moveDir) * (speed * delta);
                cameraTransform.isDirty = true;
            }
        }

        bool const manualActive = manualLook || moveRequested;
        if (!manualActive && testState.cubeRoot != april::scene::NullEntity)
        {
            auto const orbitSpeed = 0.6f;
            testState.orbitAngle += orbitSpeed * delta;

            auto const radius = 12.0f;
            auto const height = 4.0f;
            auto const orbitX = std::cos(testState.orbitAngle) * radius;
            auto const orbitZ = std::sin(testState.orbitAngle) * radius;

            auto& cubeTransform = registry.get<april::scene::TransformComponent>(testState.cubeRoot);
            auto const targetPos = cubeTransform.localPosition;

            cameraTransform.localPosition = {targetPos.x + orbitX, targetPos.y + height, targetPos.z + orbitZ};

            auto const forward = glm::normalize(april::float3{targetPos.x - cameraTransform.localPosition.x,
                                                             targetPos.y - cameraTransform.localPosition.y,
                                                             targetPos.z - cameraTransform.localPosition.z});
            testState.yaw = std::atan2(forward.x, -forward.z);
            testState.pitch = std::asin(std::clamp(forward.y, -0.99f, 0.99f));

            cameraTransform.localRotation = {testState.pitch, testState.yaw, 0.0f};
            cameraTransform.isDirty = true;
        }

        if (!testState.switched && testState.elapsed >= 2.0f)
        {
            if (testState.cameraMain == april::scene::NullEntity || testState.cameraAlt == april::scene::NullEntity)
            {
                return;
            }

            auto& mainTag = registry.get<april::scene::TagComponent>(testState.cameraMain);
            auto& altTag = registry.get<april::scene::TagComponent>(testState.cameraAlt);
            mainTag.tag = "CameraA";
            altTag.tag = "MainCamera";
            testState.switched = true;
        }
    };

    april::Engine engine(config, hooks);
    return engine.run();
}
