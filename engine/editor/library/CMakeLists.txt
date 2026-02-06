include(${CMAKE_SOURCE_DIR}/cmake/tools.cmake)

set(IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

if(EXISTS "${IMGUI_ROOT}/imgui.h")
    set(IMGUI_SOURCES
        "${IMGUI_ROOT}/imgui.cpp"
        "${IMGUI_ROOT}/imgui_demo.cpp"
        "${IMGUI_ROOT}/imgui_draw.cpp"
        "${IMGUI_ROOT}/imgui_tables.cpp"
        "${IMGUI_ROOT}/imgui_widgets.cpp"
    )

    set(IMGUI_HEADERS
        "${IMGUI_ROOT}/imconfig.h"
        "${IMGUI_ROOT}/imgui.h"
        "${IMGUI_ROOT}/imgui_internal.h"
        "${IMGUI_ROOT}/imstb_rectpack.h"
        "${IMGUI_ROOT}/imstb_textedit.h"
        "${IMGUI_ROOT}/imstb_truetype.h"
    )

    list(APPEND IMGUI_SOURCES "${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp")
    list(APPEND IMGUI_HEADERS "${IMGUI_ROOT}/backends/imgui_impl_glfw.h")

    add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})

    target_include_directories(imgui PUBLIC "${IMGUI_ROOT}" "${IMGUI_ROOT}/backends")

    target_link_libraries(imgui PRIVATE glfw)

    if(TARGET ${CURRENT_MODULE_TARGET})
        message(STATUS "[Library] Linking 'imgui' to '${CURRENT_MODULE_TARGET}'")
        target_link_libraries(${CURRENT_MODULE_TARGET} PUBLIC imgui)
    endif()
endif()

set(IMPLOT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/implot")

if(EXISTS "${IMPLOT_ROOT}/implot.h")
    set(IMPLOT_SOURCES
        "${IMPLOT_ROOT}/implot.cpp"
        "${IMPLOT_ROOT}/implot_demo.cpp"
        "${IMPLOT_ROOT}/implot_items.cpp"
    )

    set(IMPLOT_HEADERS
        "${IMPLOT_ROOT}/implot.h"
        "${IMPLOT_ROOT}/implot_internal.h"
    )

    add_library(implot STATIC ${IMPLOT_SOURCES} ${IMPLOT_HEADERS})
    target_include_directories(implot PUBLIC "${IMPLOT_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(implot PUBLIC imgui)

    if(TARGET ${CURRENT_MODULE_TARGET})
        message(STATUS "[Library] Linking 'implot' to '${CURRENT_MODULE_TARGET}'")
        target_link_libraries(${CURRENT_MODULE_TARGET} PUBLIC implot)
    endif()
endif()
