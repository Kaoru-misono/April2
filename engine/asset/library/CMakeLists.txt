# stb_image (header-only library)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Link to the current module
if(TARGET ${CURRENT_MODULE_TARGET})
    target_link_libraries(${CURRENT_MODULE_TARGET} PRIVATE stb)
endif()

april_helper_add_library(meshoptimizer ${CURRENT_MODULE_TARGET})
