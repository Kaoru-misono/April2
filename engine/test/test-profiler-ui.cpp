#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <ui/element/element-profiler.hpp>
#include <imgui.h>
#include <implot/implot.h>

TEST_CASE("ElementProfiler Instantiation") {
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Just verify we can create it
    auto profiler = std::make_shared<april::ui::ElementProfiler>();
    CHECK(profiler != nullptr);

    // Test V-Sync placeholder
    // Default should be safe (maybe false?)
    // This will fail until I implement them
    profiler->setVsync(true);
    // Since it's a placeholder without backing storage in the original spec,
    // I need to decide if it stores state. The spec said "placeholder UI toggle hooking into placeholder setVsync".
    // I will assume it should at least be callable.
    // If I want to test state, I should implement a simple member var for now.

    // Let's just verify existence by calling them.
    profiler->setVsync(true);
    (void)profiler->isVsync();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}
