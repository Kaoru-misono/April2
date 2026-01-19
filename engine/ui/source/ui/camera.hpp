#pragma once
#include <filesystem>
#include <memory>

#include "core/tools/camera-manipulator.hpp"

namespace april::ui
{
    // Bitset enum for controlling which camera widget sections are open by default
    enum CameraWidgetSections : uint32_t
    {
        CameraSection_Position   = 1 << 0, // Position section (eye, center, up)
        CameraSection_Projection = 1 << 1, // Projection section (FOV, clip planes)
        CameraSection_Other      = 1 << 2, // Other section (up vector, transition)

        // Convenience combinations
        CameraSection_None    = 0,
        CameraSection_All     = CameraSection_Position | CameraSection_Projection | CameraSection_Other,
        CameraSection_Default = CameraSection_Projection // Current behavior - only projection open
    };

    // Shows GUI for april::core::CameraManipulator.
    // If `embed` is true, it will have text before it and appear in ImGui::BeginChild.
    // `openSections` controls which sections are open by default.
    // Returns whether camera parameters changed.
    auto CameraWidget(std::shared_ptr<april::core::CameraManipulator> cameraManip,
                      bool                                            embed        = false,
                      CameraWidgetSections                            openSections = CameraSection_Default) -> bool;

    // Sets the name (without .json) of the setting file. It will load and replace all cameras and settings
    auto SetCameraJsonFile(const std::filesystem::path& filename) -> void;
    // Sets the home camera - replacing the one on load
    auto SetHomeCamera(const april::core::CameraManipulator::Camera& camera) -> void;
    // Adds a camera to the list of cameras
    auto AddCamera(const april::core::CameraManipulator::Camera& camera) -> void;

} // namespace april::ui
