#include "fonts.hpp"

#include <imgui.h>

#include "external/fonts/roboto_mono.h"
#include "external/fonts/roboto_regular.h"
#include "external/fonts/material_symbols_rounded_regular.h"
#include "external/IconsMaterialSymbols.h" // ICON_MS definitions

#define MATERIAL_SYMBOLS_DATA g_materialSymbolsRounded_compressed_data
#define MATERIAL_SYMBOLS_SIZE g_materialSymbolsRounded_compressed_size

namespace april::ui
{
    inline namespace
    {
        ImFont* g_defaultFont   = nullptr;
        ImFont* g_iconicFont    = nullptr;
        ImFont* g_monospaceFont = nullptr;
    }

    static auto getDefaultConfig() -> ImFontConfig
    {
        ImFontConfig config{};
        config.OversampleH = 3;
        config.OversampleV = 3;
        return config;
    }

    // Helper function to append a font with embedded Material Symbols icons
    // Icon fonts: https://fonts.google.com/icons?icon.set=Material+Symbols
    static auto appendFontWithMaterialSymbols(const void* fontData, int fontDataSize, float fontSize) -> ImFont*
    {
        // Configure Material Symbols icon font for merging
        ImFontConfig iconConfig = getDefaultConfig();
        iconConfig.MergeMode    = true;
        iconConfig.PixelSnapH   = true;

        // Material Symbols specific configuration
        float iconFontSize       = 1.28571429f * fontSize; // Material Symbols work best at 9/7x the base font size
        iconConfig.GlyphOffset.x = iconFontSize * 0.01f;
        iconConfig.GlyphOffset.y = iconFontSize * 0.2f;

        // Define the Material Symbols character range
        static const ImWchar materialSymbolsRange[] = {ICON_MIN_MS, ICON_MAX_MS, 0};

        // Load embedded Material Symbols
        ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData, fontDataSize, iconFontSize, &iconConfig, materialSymbolsRange);

        return font;
    }

    // Add default Roboto fonts with the option to merge Material Symbols (icons)
    auto addDefaultFont(float fontSize, bool appendIcons) -> void
    {
        if (g_defaultFont == nullptr)
        {
            ImFontConfig fontConfig = getDefaultConfig();
            g_defaultFont           = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(g_roboto_regular_compressed_data,
                                                                                 g_roboto_regular_compressed_size, fontSize, &fontConfig);

            if (appendIcons) // If appendIcons is true, merge Material Symbols into the default font
            {
                g_defaultFont = appendFontWithMaterialSymbols(MATERIAL_SYMBOLS_DATA, MATERIAL_SYMBOLS_SIZE, fontSize);
            }
        }
    }

    auto getDefaultFont() -> ImFont*
    {
        return g_defaultFont;
    }

    auto addMonospaceFont(float fontSize) -> void
    {
        if (g_monospaceFont == nullptr)
        {
            ImFontConfig fontConfig = getDefaultConfig();
            g_monospaceFont         = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
                g_roboto_mono_compressed_data,
                g_roboto_mono_compressed_size,
                fontSize,
                &fontConfig
            );
        }
    }

    auto getMonospaceFont() -> ImFont*
    {
        return g_monospaceFont;
    }

} // namespace april::ui
