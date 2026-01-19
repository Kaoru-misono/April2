#include "ui/camera.hpp"
#include "core/log/logger.hpp"
#include "core/file/file-utils.hpp"
#include "ui/property-editor.hpp"
#include "ui/tools/tooltip.hpp"
#include "ui/font/fonts.hpp"

#include "core/tools/camera-manipulator.hpp"

#include <imgui.h>
#include <json.hpp>

#include <fstream>
#include <sstream>
#include <format>


#ifdef _MSC_VER
#define SAFE_SSCANF sscanf_s
#else
#define SAFE_SSCANF sscanf
#endif

using nlohmann::json;

namespace PE = april::ui::PropertyEditor;

namespace april::ui
{
    struct CameraPresetManager
    {
        CameraPresetManager() {}
        ~CameraPresetManager() {};

        static auto getInstance() -> CameraPresetManager&
        {
            static CameraPresetManager instance;
            return instance;
        }

        auto update(std::shared_ptr<april::core::CameraManipulator> cameraManip) -> void
        {
            if (m_cameras.empty())
            {
                m_cameras.emplace_back(cameraManip->getCamera());
            }
            if (m_doLoadSetting)
                loadSetting(cameraManip);

            auto& IO = ImGui::GetIO();
            if (m_settingsDirtyTimer > 0.0f)
            {
                m_settingsDirtyTimer -= IO.DeltaTime;
                if (m_settingsDirtyTimer <= 0.0f)
                {
                    saveSetting(cameraManip);
                    m_settingsDirtyTimer = 0.0f;
                }
            }
        }

        auto removedSavedCameras() -> void
        {
            if (m_cameras.size() > 1)
                m_cameras.erase(m_cameras.begin() + 1, m_cameras.end());
        }

        auto setCameraJsonFile(const std::filesystem::path& filename) -> void
        {
            std::filesystem::path jsonFile = april::core::getExecutablePath().parent_path() / filename.filename();
            jsonFile.replace_extension(".json");
            m_jsonFilename  = std::move(jsonFile);
            m_doLoadSetting = true;
            removedSavedCameras();
        }

        auto setHomeCamera(const april::core::CameraManipulator::Camera& camera) -> void
        {
            if (m_cameras.empty())
                m_cameras.resize(1);
            m_cameras[0] = camera;
        }

        auto addCamera(const april::core::CameraManipulator::Camera& camera) -> void
        {
            bool unique = true;
            for (const april::core::CameraManipulator::Camera& c : m_cameras)
            {
                if (c == camera)
                {
                    unique = false;
                    break;
                }
            }
            if (unique)
            {
                m_cameras.emplace_back(camera);
                markJsonSettingsDirty();
            }
        }

        auto removeCamera(int delete_item) -> void
        {
            m_cameras.erase(m_cameras.begin() + delete_item);
            markJsonSettingsDirty();
        }

        auto markJsonSettingsDirty() -> void
        {
            if (m_settingsDirtyTimer <= 0.0f)
                m_settingsDirtyTimer = 0.1f;
        }

        template<typename T>
        auto getJsonValue(const json& j, const std::string& name, T& value) -> bool
        {
            auto fieldIt = j.find(name);
            if (fieldIt != j.end())
            {
                value = (*fieldIt);
                return true;
            }
            AP_WARN("Could not find JSON field {}", name.c_str());
            return false;
        }

        template<typename T>
        auto getJsonArray(const json& j, const std::string& name, T& value) -> bool
        {
            auto fieldIt = j.find(name);
            if (fieldIt != j.end())
            {
                value = T((*fieldIt).begin(), (*fieldIt).end());
                return true;
            }
            AP_WARN("Could not find JSON field {}", name.c_str());
            return false;
        }

        auto loadSetting(std::shared_ptr<april::core::CameraManipulator> cameraM) -> void
        {
            if (m_jsonFilename.empty())
            {
                m_jsonFilename = april::core::getExecutablePath().replace_extension(".json");
            }

            if (m_cameras.empty() || m_doLoadSetting == false)
                return;

            const glm::vec2& currentClipPlanes = cameraM->getClipPlanes();
            try
            {
                m_doLoadSetting = false;

                std::ifstream i(m_jsonFilename);
                if (!i.is_open())
                    return;

                json j;
                i >> j;

                int                iVal;
                float              fVal;
                std::vector<float> vfVal;

                if (getJsonValue(j, "mode", iVal))
                    cameraM->setMode(static_cast<april::core::CameraManipulator::Modes>(iVal));
                if (getJsonValue(j, "speed", fVal))
                    cameraM->setSpeed(fVal);
                if (getJsonValue(j, "anim_duration", fVal))
                    cameraM->setAnimationDuration(fVal);

                std::vector<json> cc;
                getJsonArray(j, "cameras", cc);
                for (auto& c : cc)
                {
                    april::core::CameraManipulator::Camera camera;
                    if (getJsonArray(c, "eye", vfVal))
                        camera.eye = {vfVal[0], vfVal[1], vfVal[2]};
                    if (getJsonArray(c, "ctr", vfVal))
                        camera.ctr = {vfVal[0], vfVal[1], vfVal[2]};
                    if (getJsonArray(c, "up", vfVal))
                        camera.up = {vfVal[0], vfVal[1], vfVal[2]};
                    if (getJsonValue(c, "fov", fVal))
                        camera.fov = fVal;
                    if (getJsonArray(c, "clip", vfVal))
                        camera.clip = {vfVal[0], vfVal[1]};
                    else
                        camera.clip = currentClipPlanes;
                    addCamera(camera);
                }
                i.close();
            }
            catch (...)
            {
                return;
            }
        }

        auto saveSetting(std::shared_ptr<april::core::CameraManipulator>& cameraManip) -> void
        {
            if (m_jsonFilename.empty())
                return;

            try
            {
                json j;
                j["mode"]          = cameraManip->getMode();
                j["speed"]         = cameraManip->getSpeed();
                j["anim_duration"] = cameraManip->getAnimationDuration();

                json cc = json::array();
                for (size_t n = 1; n < m_cameras.size(); n++)
                {
                    auto& c    = m_cameras[n];
                    json  jo   = json::object();
                    jo["eye"]  = std::vector<float>{c.eye.x, c.eye.y, c.eye.z};
                    jo["up"]   = std::vector<float>{c.up.x, c.up.y, c.up.z};
                    jo["ctr"]  = std::vector<float>{c.ctr.x, c.ctr.y, c.ctr.z};
                    jo["fov"]  = c.fov;
                    jo["clip"] = std::vector<float>{c.clip.x, c.clip.y};
                    cc.push_back(jo);
                }
                j["cameras"] = cc;

                std::ofstream o(m_jsonFilename);
                if (o.is_open())
                {
                    o << j.dump(2) << std::endl;
                    o.close();
                }
            }
            catch (const std::exception& e)
            {
                AP_ERROR("Could not save camera settings to {}: {}", april::core::utf8FromPath(m_jsonFilename).c_str(), e.what());
            }
        }

        std::vector<april::core::CameraManipulator::Camera> m_cameras{};
        float                                               m_settingsDirtyTimer{0};
        std::filesystem::path                               m_jsonFilename{};
        bool                                                m_doLoadSetting{true};
    };

    static float s_buttonSpacing = 4.0f;

    static auto PeBeginAutostretch(const char* label) -> bool
    {
        if (!PE::begin(label, ImGuiTableFlags_SizingFixedFit))
            return false;
        ImGui::TableSetupColumn("Property");
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    static auto QuickActionsBar(std::shared_ptr<april::core::CameraManipulator> cameraM, april::core::CameraManipulator::Camera& camera) -> bool
    {
        bool changed = false;

        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);

        if (ImGui::Button(ICON_MS_HOME))
        {
            camera  = CameraPresetManager::getInstance().m_cameras[0];
            changed = true;
        }
        Tooltip::hover("Reset to home camera position");

        ImGui::SameLine(0, s_buttonSpacing);
        if (ImGui::Button(ICON_MS_ADD_A_PHOTO))
        {
            CameraPresetManager::getInstance().addCamera(cameraM->getCamera());
        }
        Tooltip::hover("Save current camera position");

        ImGui::SameLine(0, s_buttonSpacing);
        if (ImGui::Button(ICON_MS_CONTENT_COPY))
        {
            std::string text = camera.getString();
            ImGui::SetClipboardText(text.c_str());
        }
        Tooltip::hover("Copy camera state to clipboard");

        const char* pPastedString;
        ImGui::SameLine(0, s_buttonSpacing);
        if (ImGui::Button(ICON_MS_CONTENT_PASTE) && (pPastedString = ImGui::GetClipboardText()))
        {
            std::string text(pPastedString);
            changed = camera.setFromString(text);
        }
        Tooltip::hover("Paste camera state from clipboard");

        const float button_size = ImGui::CalcTextSize(ICON_MS_HELP).x + ImGui::GetStyle().FramePadding.x * 2.f;
        ImGui::SameLine(ImGui::GetContentRegionMax().x - button_size, 0.0f);
        if (ImGui::Button(ICON_MS_HELP))
        {
            ImGui::OpenPopup("Camera Help");
        }
        Tooltip::hover("Show camera controls help");

        ImGui::PopStyleColor();

        if (ImGui::BeginPopupModal("Camera Help", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Camera Controls:");
            ImGui::BulletText("Left Mouse: Orbit/Pan/Dolly (depends on mode)");
            ImGui::BulletText("Right Mouse: Look around");
            ImGui::BulletText("Middle Mouse: Pan");
            ImGui::BulletText("Mouse Wheel: Zoom (change FOV)");
            ImGui::BulletText("WASD: Move camera");
            ImGui::BulletText("Q/E: Roll camera");
            ImGui::Spacing();
            ImGui::Text("Navigation Modes:");
            ImGui::BulletText("Examine: Orbit around center point");
            ImGui::BulletText("Fly: Free movement in 3D space");
            ImGui::BulletText("Walk: Movement constrained to horizontal plane");

            if (ImGui::Button("Close", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        return changed;
    }

    static auto PresetsSection(std::shared_ptr<april::core::CameraManipulator> cameraM, april::core::CameraManipulator::Camera& camera) -> bool
    {
        bool changed = false;

        auto& presetManager   = CameraPresetManager::getInstance();
        int   buttonsCount    = (int)presetManager.m_cameras.size();
        float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

        if (buttonsCount == 1)
            ImGui::TextDisabled(" - No saved cameras");

        int         delete_item = -1;
        std::string thisLabel   = "#1";
        std::string nextLabel;
        for (int n = 1; n < buttonsCount; n++)
        {
            nextLabel = std::format("#{}", n + 1);
            ImGui::PushID(n);
            if (ImGui::Button(thisLabel.c_str()))
            {
                camera  = presetManager.m_cameras[n];
                changed = true;
            }

            if (ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[ImGuiMouseButton_Middle])
                delete_item = n;

            const auto& cam = presetManager.m_cameras[n];
            std::string tooltip =
                std::format("Camera #{}\n({:.1f}, {:.1f}, {:.1f})\nMiddle click to delete", n, cam.eye.x, cam.eye.y, cam.eye.z);
            Tooltip::hover(tooltip.c_str());

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 =
                last_button_x2 + s_buttonSpacing + ImGui::CalcTextSize(nextLabel.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.f;
            if (n + 1 < buttonsCount && next_button_x2 < windowVisibleX2)
                ImGui::SameLine(0, s_buttonSpacing);

            thisLabel = std::move(nextLabel);

            ImGui::PopID();
        }

        if (delete_item > 0)
        {
            presetManager.removeCamera(delete_item);
        }

        return changed;
    }

    static auto NavigationSettingsSection(std::shared_ptr<april::core::CameraManipulator> cameraM) -> bool
    {
        bool changed = false;

        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.f);

        auto mode     = cameraM->getMode();
        auto speed    = cameraM->getSpeed();
        auto duration = static_cast<float>(cameraM->getAnimationDuration());

        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);

        auto setColor = [&](bool selected) {
            ImGui::GetStyle().Colors[ImGuiCol_Button] =
                selected ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
        };

        setColor(mode == april::core::CameraManipulator::Examine);
        if (ImGui::Button(ICON_MS_ORBIT))
        {
            cameraM->setMode(april::core::CameraManipulator::Examine);
            changed = true;
        }
        Tooltip::hover("Orbit around a point of interest");
        ImGui::SameLine(0, s_buttonSpacing);
        setColor(mode == april::core::CameraManipulator::Fly);
        if (ImGui::Button(ICON_MS_FLIGHT))
        {
            cameraM->setMode(april::core::CameraManipulator::Fly);
            changed = true;
        }
        Tooltip::hover("Fly: Free camera movement");
        ImGui::SameLine(0, s_buttonSpacing);
        setColor(mode == april::core::CameraManipulator::Walk);
        if (ImGui::Button(ICON_MS_DIRECTIONS_WALK))
        {
            cameraM->setMode(april::core::CameraManipulator::Walk);
            changed = true;
        }
        Tooltip::hover("Walk: Stay on a horizontal plane");

        ImGui::PopStyleColor();
        const bool showSettings = (mode == april::core::CameraManipulator::Fly || mode == april::core::CameraManipulator::Walk);
        if (showSettings)
        {
            if (PeBeginAutostretch("##Speed"))
            {
                const float speedMin = 1e-3f;
                const float speedMax = 1e+3f;
                changed |= PE::DragFloat("Speed", &speed, 2e-4f * (speedMax - speedMin), speedMin, speedMax, "%.2f",
                                         ImGuiSliderFlags_Logarithmic, "Speed of camera movement");
                cameraM->setSpeed(speed);
                PE::end();
            }
        }

        return false;
    }

    static auto PositionSection(std::shared_ptr<april::core::CameraManipulator> cameraM,
                                april::core::CameraManipulator::Camera&         camera,
                                ImGuiTreeNodeFlags                              flag = ImGuiTreeNodeFlags_None) -> bool
    {
        bool changed = false;

        bool myChanged = false;

        if (ImGui::TreeNodeEx("Position", flag))
        {
            if (PeBeginAutostretch("##Position"))
            {
                myChanged |= PE::InputFloat3("Eye", &camera.eye.x);
                myChanged |= PE::InputFloat3("Center", &camera.ctr.x);
                myChanged |= PE::InputFloat3("Up", &camera.up.x);
                PE::end();
            }
            ImGui::TreePop();
        }

        if (!cameraM->isAnimated())
        {
            changed |= myChanged;
        }

        return changed;
    }

    static auto ProjectionSettingsSection(std::shared_ptr<april::core::CameraManipulator> cameraManip,
                                          april::core::CameraManipulator::Camera&         camera,
                                          ImGuiTreeNodeFlags                              flag = ImGuiTreeNodeFlags_None) -> bool
    {
        bool changed = false;
        if (ImGui::TreeNodeEx("Projection", flag))
        {
            if (PeBeginAutostretch("##Projection"))
            {
                changed |= PE::SliderFloat("FOV", &camera.fov, 1.F, 179.F, "%.1f°", ImGuiSliderFlags_Logarithmic,
                                           "Field of view of the camera (degrees)");

                const float minClip = 1e-5f;
                const float maxClip = 1e+9f;
                changed |= PE::DragFloat2("Z-Clip", &camera.clip.x, 2e-5f * (maxClip - minClip), minClip, maxClip, "%.6f",
                                          ImGuiSliderFlags_Logarithmic, "Near/Far clip planes for depth buffer");

                PE::end();
            }
            ImGui::TreePop();
        }

        return changed;
    }

    static auto OtherSettingsSection(std::shared_ptr<april::core::CameraManipulator> cameraM,
                                     april::core::CameraManipulator::Camera&         camera,
                                     ImGuiTreeNodeFlags                              flag = ImGuiTreeNodeFlags_None) -> bool
    {
        bool changed = false;
        if (ImGui::TreeNodeEx("Other", flag))
        {
            if (PeBeginAutostretch("##Other"))
            {
                PE::entry("Up vector", [&] {
                    const bool yIsUp = camera.up.y == 1;
                    if (ImGui::RadioButton("Y-up", yIsUp))
                    {
                        camera.up = glm::vec3(0, 1, 0);
                        changed   = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Z-up", !yIsUp))
                    {
                        camera.up = glm::vec3(0, 0, 1);
                        changed   = true;
                    }
                    if (glm::length(camera.up) < 0.0001f)
                    {
                        camera.up = yIsUp ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
                        changed   = true;
                    }
                    return changed;
                });

                auto duration = static_cast<float>(cameraM->getAnimationDuration());
                changed |= PE::SliderFloat("Transition", &duration, 0.0F, 2.0F, "%.2fs", ImGuiSliderFlags_None,
                                           "Transition duration of camera movement");
                cameraM->setAnimationDuration(duration);

                PE::end();
            }
            ImGui::TreePop();
        }

        return changed;
    }

    auto CameraWidget(std::shared_ptr<april::core::CameraManipulator> cameraManip, bool embed, CameraWidgetSections openSections) -> bool
    {
        assert(cameraManip && "CameraManipulator is not set");

        bool changed{false};
        bool instantChanged{false};

        april::core::CameraManipulator::Camera camera = cameraManip->getCamera();

        CameraPresetManager::getInstance().update(cameraManip);

        if (embed)
        {
            ImGui::Text("Camera Settings");
            if (!ImGui::BeginChild("CameraPanel", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
            {
                return false;
            }
        }

        {
            changed |= QuickActionsBar(cameraManip, camera);
            changed |= PresetsSection(cameraManip, camera);
            changed |= NavigationSettingsSection(cameraManip);
            ImGui::Separator();
            instantChanged |= ProjectionSettingsSection(cameraManip, camera,
                                                        (openSections & CameraSection_Projection) ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
            changed |= PositionSection(cameraManip, camera,
                                       (openSections & CameraSection_Position) ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
            changed |= OtherSettingsSection(cameraManip, camera,
                                            (openSections & CameraSection_Other) ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
            if (embed)
            {
                ImGui::EndChild();
            }
        }

        if (changed || instantChanged)
        {
            CameraPresetManager::getInstance().markJsonSettingsDirty();
            cameraManip->setCamera(camera, instantChanged);
        }

        return changed || instantChanged;
    }

    auto SetCameraJsonFile(const std::filesystem::path& filename) -> void
    {
        CameraPresetManager::getInstance().setCameraJsonFile(filename);
    }

    auto SetHomeCamera(const april::core::CameraManipulator::Camera& camera) -> void
    {
        CameraPresetManager::getInstance().setHomeCamera(camera);
    }

    auto AddCamera(const april::core::CameraManipulator::Camera& camera) -> void
    {
        CameraPresetManager::getInstance().addCamera(camera);
    }
} // namespace april::ui
