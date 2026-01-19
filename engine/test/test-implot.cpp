#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <implot/implot.h>
#include <imgui.h>

TEST_CASE("ImPlot Context Creation") {
    ImGui::CreateContext();
    ImPlot::CreateContext();

    CHECK(ImPlot::GetCurrentContext() != nullptr);

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}
