set(TARGET_NAME APP)

# find_package(Vulkan 1.4.330...1.4.341 REQUIRED)

# file(GLOB_RECURSE SRC_FILES "source/asset-system-test.cpp")
file(GLOB_RECURSE SRC_FILES "test-triangle.cpp")

add_executable(APP test-triangle.cpp)
target_link_libraries(APP PRIVATE April_graphics)

# add_executable(test-swapchain-resize test-swapchain-resize.cpp)
# target_link_libraries(test-swapchain-resize PRIVATE Vulkan::Vulkan glfw slang)
# target_compile_features(test-swapchain-resize PRIVATE cxx_std_23)
# target_compile_definitions(test-swapchain-resize PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

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
target_link_libraries(test-ui-font PRIVATE April_graphics April_editor)
target_compile_features(test-ui-font PRIVATE cxx_std_23)
target_compile_definitions(test-ui-font PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

add_executable(test-nv-template test-nv-template.cpp)
target_link_libraries(test-nv-template PRIVATE April_graphics April_editor)
target_compile_features(test-nv-template PRIVATE cxx_std_23)
target_compile_definitions(test-nv-template PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

add_executable(test-implot test-implot.cpp)
target_include_directories(test-implot PRIVATE external/doctest)
target_link_libraries(test-implot PRIVATE April_graphics implot)
target_compile_features(test-implot PRIVATE cxx_std_23)
target_compile_definitions(test-implot PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

add_executable(test-input
    test-main.cpp
    test-input.cpp
)
target_include_directories(test-input PRIVATE external/doctest)
target_link_libraries(test-input PRIVATE April_core)
target_compile_features(test-input PRIVATE cxx_std_23)
target_compile_definitions(test-input PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)


# Profiler Test Unit
add_executable(test-profiler-full
    test-main.cpp
    test-profiler.cpp
    test-gpu-profiler.cpp
    test-thread-registry.cpp
    test-profile-exporter.cpp
)
target_include_directories(test-profiler-full PRIVATE external/doctest)
target_link_libraries(test-profiler-full PRIVATE April_graphics)
target_compile_features(test-profiler-full PRIVATE cxx_std_23)
target_compile_definitions(test-profiler-full PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

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
    source/test-blit-reflection.cpp
)
target_include_directories(test-suite PRIVATE external/doctest)
target_link_libraries(test-suite PRIVATE April_graphics)
target_compile_features(test-suite PRIVATE cxx_std_23)
target_compile_definitions(test-suite PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

# Standalone RHI Validation Test
add_executable(test-rhi-validation
    test-main.cpp
    source/test-rhi-validation.cpp
)
target_include_directories(test-rhi-validation PRIVATE external/doctest)
target_link_libraries(test-rhi-validation PRIVATE April_graphics)
target_compile_features(test-rhi-validation PRIVATE cxx_std_23)
target_compile_definitions(test-rhi-validation PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

# Standalone Blit Test
add_executable(test-blit
    test-main.cpp
    test-blit.cpp
)
target_include_directories(test-blit PRIVATE external/doctest)
target_link_libraries(test-blit PRIVATE April_graphics)
target_compile_features(test-blit PRIVATE cxx_std_23)
target_compile_definitions(test-blit PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

# Standalone Blit Uniform Readback Test
add_executable(test-blit-uniform-readback
    test-main.cpp
    source/test-blit-uniform-readback.cpp
)
target_include_directories(test-blit-uniform-readback PRIVATE external/doctest)
target_link_libraries(test-blit-uniform-readback PRIVATE April_graphics)
target_compile_features(test-blit-uniform-readback PRIVATE cxx_std_23)
target_compile_definitions(test-blit-uniform-readback PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

# Standalone UV Coverage Test
add_executable(test-uv-coverage
    test-main.cpp
    source/test-uv-coverage.cpp
)
target_include_directories(test-uv-coverage PRIVATE external/doctest)
target_link_libraries(test-uv-coverage PRIVATE April_graphics)
target_compile_features(test-uv-coverage PRIVATE cxx_std_23)
target_compile_definitions(test-uv-coverage PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

add_executable(test-asset test-asset.cpp)
target_link_libraries(test-asset PRIVATE April_asset)
target_include_directories(test-asset PRIVATE external/doctest)
target_compile_features(test-asset PRIVATE cxx_std_23)
target_compile_definitions(test-asset PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)
