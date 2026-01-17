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

add_executable(test-imgui test-imgui.cpp)
target_include_directories(test-imgui PRIVATE external/doctest)
target_link_libraries(test-imgui PRIVATE April_graphics)
target_compile_features(test-imgui PRIVATE cxx_std_23)
target_compile_definitions(test-imgui PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

add_executable(test-imgui-backend test-imgui-backend.cpp)
target_include_directories(test-imgui-backend PRIVATE external/doctest)
target_link_libraries(test-imgui-backend PRIVATE April_graphics)
target_compile_features(test-imgui-backend PRIVATE cxx_std_23)
target_compile_definitions(test-imgui-backend PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)

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
)
target_include_directories(test-suite PRIVATE external/doctest)
target_link_libraries(test-suite PRIVATE April_graphics)
target_compile_features(test-suite PRIVATE cxx_std_23)
target_compile_definitions(test-suite PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE)
