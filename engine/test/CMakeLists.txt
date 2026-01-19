set(TARGET_NAME APP)

# file(GLOB_RECURSE SRC_FILES "source/asset-system-test.cpp")
file(GLOB_RECURSE SRC_FILES "test-triangle.cpp")

add_executable(APP test-triangle.cpp)
target_link_libraries(APP PRIVATE April_graphics)

add_executable(test-object test-object.cpp)
target_link_libraries(test-object PRIVATE April_graphics)

add_executable(test-resource test-resource.cpp)
target_link_libraries(test-resource PRIVATE April_graphics)

add_executable(test-slang-rhi test-slang-rhi.cpp)
target_link_libraries(test-slang-rhi PRIVATE April_graphics)

target_compile_features(APP PRIVATE cxx_std_23)
target_compile_features(test-object PRIVATE cxx_std_23)
target_compile_features(test-resource PRIVATE cxx_std_23)
target_compile_features(test-slang-rhi PRIVATE cxx_std_23)

target_compile_definitions(APP PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)
target_compile_definitions(test-object PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)
target_compile_definitions(test-resource PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)
target_compile_definitions(test-slang-rhi PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

add_executable(test-ui-font test-ui-font.cpp)
target_include_directories(test-ui-font PRIVATE external/doctest)
target_link_libraries(test-ui-font PRIVATE April_graphics April_ui)
target_compile_features(test-ui-font PRIVATE cxx_std_23)
target_compile_definitions(test-ui-font PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

add_executable(test-nv-template test-nv-template.cpp)
target_link_libraries(test-nv-template PRIVATE April_graphics April_ui)
target_compile_features(test-nv-template PRIVATE cxx_std_23)
target_compile_definitions(test-nv-template PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

# Unified Test Suite
add_executable(test-suite
    test-main.cpp
    external/doctest/doctest/doctest.h
    source/test-compilation.cpp
    source/test-reflection.cpp
    source/test-binding.cpp
    source/test-reloading.cpp
    source/test-conformance.cpp
    source/test-advanced.cpp
    source/test-rhi-validation.cpp
    source/test-log-foundation.cpp
    source/test-logger-refactor.cpp
    source/test-log-enhancement.cpp
)
target_include_directories(test-suite PRIVATE external/doctest)
target_link_libraries(test-suite PRIVATE April_graphics)
target_compile_features(test-suite PRIVATE cxx_std_23)
target_compile_definitions(test-suite PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)
