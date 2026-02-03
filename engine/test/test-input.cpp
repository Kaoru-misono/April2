#include <doctest/doctest.h>

#include <core/input/input.hpp>

namespace
{
    constexpr int kTestKey = 65;
    constexpr int kTestMouseButton = 1;
}

TEST_CASE("Input key transitions")
{
    april::Input::beginFrame();
    april::Input::setKeyDown(kTestKey, true);
    CHECK(april::Input::isKeyDown(kTestKey));
    CHECK(april::Input::wasKeyPressed(kTestKey));
    CHECK(!april::Input::wasKeyReleased(kTestKey));

    april::Input::beginFrame();
    CHECK(april::Input::isKeyDown(kTestKey));
    CHECK(!april::Input::wasKeyPressed(kTestKey));

    april::Input::setKeyDown(kTestKey, false);
    CHECK(!april::Input::isKeyDown(kTestKey));
    CHECK(april::Input::wasKeyReleased(kTestKey));
}

TEST_CASE("Input mouse transitions")
{
    april::Input::beginFrame();
    april::Input::setMouseButtonDown(kTestMouseButton, true);
    CHECK(april::Input::isMouseDown(kTestMouseButton));
    CHECK(april::Input::wasMousePressed(kTestMouseButton));

    april::Input::beginFrame();
    CHECK(april::Input::isMouseDown(kTestMouseButton));
    CHECK(!april::Input::wasMousePressed(kTestMouseButton));

    april::Input::setMouseButtonDown(kTestMouseButton, false);
    CHECK(!april::Input::isMouseDown(kTestMouseButton));
    CHECK(april::Input::wasMouseReleased(kTestMouseButton));
}

TEST_CASE("Input mouse wheel")
{
    april::Input::beginFrame();
    april::Input::addMouseWheel(april::float2{1.0f, -2.0f});
    auto delta = april::Input::getMouseWheelDelta();
    CHECK(delta.x == doctest::Approx(1.0f));
    CHECK(delta.y == doctest::Approx(-2.0f));

    april::Input::beginFrame();
    delta = april::Input::getMouseWheelDelta();
    CHECK(delta.x == doctest::Approx(0.0f));
    CHECK(delta.y == doctest::Approx(0.0f));
}

TEST_CASE("Input UI capture and focus")
{
    april::Input::setUiCapture(true, false);
    CHECK(april::Input::isMouseCapturedByUi());
    CHECK(!april::Input::shouldProcessMouse());
    CHECK(!april::Input::isKeyboardCapturedByUi());
    CHECK(april::Input::shouldProcessKeyboard());

    april::Input::setWindowFocused(false);
    CHECK(!april::Input::isWindowFocused());
    april::Input::setWindowFocused(true);
    CHECK(april::Input::isWindowFocused());
}
