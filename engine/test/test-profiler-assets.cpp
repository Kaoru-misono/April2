#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <imgui.h>
#include <ui/font/fonts.hpp>
#include <ui/font/external/IconsMaterialSymbols.h>
#include <string>

TEST_CASE("Profiler Assets Integration") {
    // Verify IconsMaterialSymbols.h is usable (compile-time check mainly, but we can check a macro)
    CHECK(ICON_MIN_MS > 0);
    CHECK(ICON_MAX_MS > 0);
    // Check a specific icon used in the profiler
    CHECK(std::string(ICON_MS_BLOOD_PRESSURE).empty() == false);
    CHECK(std::string(ICON_MS_CONTENT_COPY).empty() == false);

    // Verify Monospace Font
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Initially it might be null if not added
    // But since we are testing integration, we want to ensure we CAN get it.

    // We need to call addMonospaceFont to load it.
    // Assuming 16.0f is a safe size.
    april::ui::addMonospaceFont(16.0f);

    // Now it should be valid
    ImFont* mono = april::ui::getMonospaceFont();
    CHECK(mono != nullptr);

    // Ensure it was added to the atlas
    bool found = false;
    for (int i = 0; i < io.Fonts->Fonts.Size; i++) {
        if (io.Fonts->Fonts[i] == mono) {
            found = true;
            break;
        }
    }
    CHECK(found);

    ImGui::DestroyContext();
}

#include <ui/element/element-profiler.hpp>

TEST_CASE("ElementProfiler V-Sync Placeholder") {
    auto profiler = std::make_shared<april::ui::ElementProfiler>();
    
    // Default should be true or false (check implementation default)
    // In code: bool m_vsync = true;
    CHECK(profiler->isVsync() == true);

    profiler->setVsync(false);
    CHECK(profiler->isVsync() == false);

    profiler->setVsync(true);
    CHECK(profiler->isVsync() == true);
}
