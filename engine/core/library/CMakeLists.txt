include(${CMAKE_SOURCE_DIR}/cmake/tools.cmake)

april_helper_add_library(glfw ${CURRENT_MODULE_TARGET})
april_helper_add_library(glm ${CURRENT_MODULE_TARGET})
april_helper_add_library(tinygltf ${CURRENT_MODULE_TARGET})
april_helper_add_library(stduuid ${CURRENT_MODULE_TARGET})

add_subdirectory(json EXCLUDE_FROM_ALL)
if(TARGET ${CURRENT_MODULE_TARGET})
    message(STATUS "[Library] Linking 'nlohmann_json' to '${CURRENT_MODULE_TARGET}'")
    target_link_libraries(${CURRENT_MODULE_TARGET} PUBLIC nlohmann_json)
endif()

