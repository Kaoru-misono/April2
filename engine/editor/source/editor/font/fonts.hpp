#pragma once

struct ImFont;

namespace april::ui
{
    auto addDefaultFont(float fontSize = 15.0f, bool appendIcons = true) -> void; // Initializes the default font.
    auto getDefaultFont() -> ImFont*;                                             // Returns the default font.
    auto addMonospaceFont(float fontSize = 15.0f) -> void;                        // Initializes the monospace font.
    auto getMonospaceFont() -> ImFont*;                                           // Returns the monospace font
    auto addIconFont(float fontSize = 128.0f) -> void;                            // Initializes a large icon-only font.
    auto getIconFont() -> ImFont*;                                                // Returns the large icon font.
} // namespace april::ui
