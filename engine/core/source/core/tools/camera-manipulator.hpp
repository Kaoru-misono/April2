/*
 * Copyright (c) 2018-2025, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2018-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */
//--------------------------------------------------------------------

#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace april::core
{
    /*-------------------------------------------------------------------------------------------------
      # class april::core::CameraManipulator

      april::core::CameraManipulator is a camera manipulator help class
      It allow to simply do
      - Orbit        (LMB)
      - Pan          (LMB + CTRL  | MMB)
      - Dolly        (LMB + SHIFT | RMB)
      - Look Around  (LMB + ALT   | LMB + CTRL + SHIFT)

      In a various ways:
      - examiner(orbit around object)
      - walk (look up or down but stays on a plane)
      - fly ( go toward the interest point)
    -------------------------------------------------------------------------------------------------*/

    class CameraManipulator
    {
    public:
        CameraManipulator();

        // clang-format off
        enum Modes { Examine, Fly, Walk};
        enum Actions { NoAction, Orbit, Dolly, Pan, LookAround };
        struct Inputs {bool lmb=false; bool mmb=false; bool rmb=false; 
                       bool shift=false; bool ctrl=false; bool alt=false;};
        // clang-format on

        struct Camera
        {
            glm::vec3 eye  = glm::vec3(10, 10, 10);
            glm::vec3 ctr  = glm::vec3(0, 0, 0);
            glm::vec3 up   = glm::vec3(0, 1, 0);
            float     fov  = 60.0f;
            glm::vec2 clip = {0.001f, 100000.0f};

            bool operator!=(const Camera& rhr) const
            {
                return (eye != rhr.eye) || (ctr != rhr.ctr) || (up != rhr.up) || (fov != rhr.fov) || (clip != rhr.clip);
            }
            bool operator==(const Camera& rhr) const
            {
                return (eye == rhr.eye) && (ctr == rhr.ctr) && (up == rhr.up) && (fov == rhr.fov) && (clip == rhr.clip);
            }

            // basic serialization, mostly for copy/paste
            auto getString() const -> std::string;
            auto setFromString(const std::string& text) -> bool;
        };

    public:
        // Main function to call from the application
        // On application mouse move, call this function with the current mouse position, mouse
        // button presses and keyboard modifier. The camera matrix will be updated and
        // can be retrieved calling getMatrix
        auto mouseMove(glm::vec2 screenDisplacement, const Inputs& inputs) -> Actions;

        // Set the camera to look at the interest point
        // instantSet = true will not interpolate to the new position
        auto setLookat(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool instantSet = true) -> void;

        // This should be called in an application loop to update the camera matrix if this one is animated: new position, key movement
        auto updateAnim() -> void;

        // To call when the size of the window change.  This allows to do nicer movement according to the window size.
        auto setWindowSize(glm::uvec2 winSize) -> void { m_windowSize = winSize; }

        auto getCamera() const -> Camera { return m_current; }
        auto setCamera(Camera camera, bool instantSet = true) -> void;

        // Retrieve the position, interest and up vector of the camera
        auto      getLookat(glm::vec3& eye, glm::vec3& center, glm::vec3& up) const -> void;
        auto      getEye() const -> glm::vec3 { return m_current.eye; }
        auto      getCenter() const -> glm::vec3 { return m_current.ctr; }
        auto      getUp() const -> glm::vec3 { return m_current.up; }

        // Set the manipulator mode, from Examiner, to walk, to fly, ...
        auto setMode(Modes mode) -> void { m_mode = mode; }

        // Retrieve the current manipulator mode
        auto getMode() const -> Modes { return m_mode; }

        // Retrieving the transformation matrix of the camera
        auto getViewMatrix() const -> const glm::mat4& { return m_matrix; }

        auto getPerspectiveMatrix() const -> const glm::mat4
        {
            glm::mat4 projMatrix = glm::perspectiveRH_ZO(getRadFov(), getAspectRatio(), m_current.clip.x, m_current.clip.y);
            projMatrix[1][1] *= -1; // Flip the Y axis
            return projMatrix;
        }

        // Set the position, interest from the matrix.
        // instantSet = true will not interpolate to the new position
        // centerDistance is the distance of the center from the eye
        auto setMatrix(const glm::mat4& mat_, bool instantSet = true, float centerDistance = 1.f) -> void;

        // Changing the default speed movement
        auto setSpeed(float speed) -> void { m_speed = speed; }

        // Retrieving the current speed
        auto getSpeed() const -> float { return m_speed; }

        // Mouse position
        auto      setMousePosition(const glm::vec2& pos) -> void { m_mouse = pos; }
        auto      getMousePosition() const -> glm::vec2 { return m_mouse; }

        // Main function which is called to apply a camera motion.
        // It is preferable to
        auto motion(const glm::vec2& screenDisplacement, Actions action = NoAction) -> void;

        // This is called when moving with keys (ex. WASD)
        auto keyMotion(glm::vec2 delta, Actions action) -> void;

        // To call when the mouse wheel change
        auto wheel(float value, const Inputs& inputs) -> void;

        // Retrieve the screen dimension
        auto getWindowSize() const -> glm::uvec2 { return m_windowSize; }
        auto getAspectRatio() const -> float { return static_cast<float>(m_windowSize.x) / static_cast<float>(m_windowSize.y); }

        // Field of view in degrees
        auto  setFov(float fovDegree) -> void;
        auto  getFov() const -> float { return m_current.fov; }
        auto  getRadFov() const -> float { return glm::radians(m_current.fov); }

        // Clip planes
        auto             setClipPlanes(glm::vec2 clip) -> void { m_current.clip = clip; }
        auto             getClipPlanes() const -> const glm::vec2& { return m_current.clip; }

        // Animation duration
        auto   getAnimationDuration() const -> double { return m_duration; }
        auto   setAnimationDuration(double val) -> void { m_duration = val; }
        auto   isAnimated() const -> bool { return m_animDone == false; }

        // Returning a default help string
        auto getHelp() -> const std::string&;

        // Fitting the camera position and interest to see the bounding box
        auto fit(const glm::vec3& boxMin, const glm::vec3& boxMax, bool instantFit = true, bool tight = false, float aspect = 1.0f) -> void;

    private:
        // Update the internal matrix.
        auto updateLookatMatrix() -> void { m_matrix = glm::lookAt(m_current.eye, m_current.ctr, m_current.up); }

        // Do panning: movement parallels to the screen
        auto pan(glm::vec2 displacement) -> void;
        // Do orbiting: rotation around the center of interest. If invert, the interest orbit around the camera position
        auto orbit(glm::vec2 displacement, bool invert = false) -> void;
        // Do dolly: movement toward the interest.
        auto dolly(glm::vec2 displacement, bool keepCenterFixed = false) -> void;


        auto getSystemTime() -> double;

        auto computeBezier(float t, glm::vec3& p0, glm::vec3& p1, glm::vec3& p2) -> glm::vec3;
        auto      findBezierPoints() -> void;

    protected:
        glm::mat4 m_matrix = glm::mat4(1);

        Camera m_current;   // Current camera position
        Camera m_goal;      // Wish camera position
        Camera m_snapshot;  // Current camera the moment a set look-at is done

        // Animation
        std::array<glm::vec3, 3> m_bezier    = {};
        double                   m_startTime = 0;
        double                   m_duration  = 0.5;
        bool                     m_animDone  = true;

        // Window size
        glm::uvec2 m_windowSize = glm::uvec2(1, 1);

        // Other
        float     m_speed = 3.f;
        glm::vec2 m_mouse = glm::vec2(0.f, 0.f);

        Modes m_mode = Examine;
    };

} // namespace april::core
