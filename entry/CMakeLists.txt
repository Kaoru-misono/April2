add_executable(game
    game/main.cpp
)
target_link_libraries(game PRIVATE April_runtime)
target_compile_features(game PRIVATE cxx_std_23)
target_compile_definitions(game PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

if(APRIL_BUILD_EDITOR)
    add_executable(editor
        editor/main.cpp
    )
    target_link_libraries(editor PRIVATE April_editor)
    target_compile_features(editor PRIVATE cxx_std_23)
    target_compile_definitions(editor PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)
endif()
