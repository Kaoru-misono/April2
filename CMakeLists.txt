cmake_minimum_required(VERSION 3.25)

project(April
    VERSION 0.1.0
    DESCRIPTION "A C++23 Game Engine"
    LANGUAGES CXX C
)

option(APRIL_BUILD_EDITOR "Build editor modules and editor executable" ON)
option(APRIL_VERBOSE "Enable verbose module scanning logs" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/EHsc)
endif()

if(MSVC)
  # Enable parallel builds, and set warning level 3 for MSVC.
  add_compile_options($<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:/MP>)
  add_compile_options($<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:/W4>)

  add_compile_options(/Zc:preprocessor)
else()
  # Remove unused sections to save space (and remove usage_* doc functions).
  # You can add -Wl,--print-gc-sections to see what was removed.
  add_link_options(-Wl,--gc-sections)
endif()

include(cmake/april-build.cmake)

set(APRIL_MODULE_ROOTS
    "${CMAKE_SOURCE_DIR}/engine"
    CACHE STRING "Semicolon-separated list of module root directories"
)

april_scan_and_load_modules(${APRIL_MODULE_ROOTS})

add_subdirectory(engine/test)
add_subdirectory(entry)
