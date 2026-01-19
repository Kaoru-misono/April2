#pragma once

#include "external/IconsMaterialSymbols.h" // ICON_MS definitions

struct ImFont;

namespace april::ui
{
    auto addDefaultFont(float fontSize = 15.0f, bool appendIcons = true) -> void; // Initializes the default font.
    auto getDefaultFont() -> ImFont*;                                             // Returns the default font.
    auto addMonospaceFont(float fontSize = 15.0f) -> void;                        // Initializes the monospace font.
    auto getMonospaceFont() -> ImFont*;                                           // Returns the monospace font
} // namespace april::ui