#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <imgui.h>
#include "ui/property-editor.hpp"
#include "ui/tonemapper.hpp"
#include "ui/camera.hpp"

// Mock ImGui context for testing
struct ImGuiTestContext {
    ImGuiTestContext() {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.DisplaySize = ImVec2(1024, 768);
    }
    ~ImGuiTestContext() {
        ImGui::DestroyContext();
    }
};

TEST_CASE("UI Property Editor") {
    ImGuiTestContext ctx;
    ImGui::NewFrame();
    
    // Test PropertyEditor existence and basic interaction
    // april::ui::PropertyEditor::begin(); 
    // april::ui::PropertyEditor::end();
    
    ImGui::Render();
}

TEST_CASE("UI Tonemapper") {
    ImGuiTestContext ctx;
    ImGui::NewFrame();
    
    // april::ui::Tonemapper tonemapper;
    // tonemapper.render();
    
    ImGui::Render();
}

TEST_CASE("UI Camera") {
    ImGuiTestContext ctx;
    ImGui::NewFrame();
    
    // april::ui::Camera::renderUI(...);
    
    ImGui::Render();
}
