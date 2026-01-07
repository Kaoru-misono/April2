set(SLANG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/slang")

add_library(slang_core SHARED IMPORTED GLOBAL)
add_library(slang::slang ALIAS slang_core)
set_target_properties(slang_core PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SLANG_ROOT}/include"
    IMPORTED_IMPLIB "${SLANG_ROOT}/lib/slang.lib"
    IMPORTED_LOCATION "${SLANG_ROOT}/bin/slang.dll"
)

add_library(slang_rt SHARED IMPORTED GLOBAL)
add_library(slang::rt ALIAS slang_rt)
set_target_properties(slang_rt PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SLANG_ROOT}/include"
    IMPORTED_IMPLIB "${SLANG_ROOT}/lib/slang-rt.lib"
    IMPORTED_LOCATION "${SLANG_ROOT}/bin/slang-rt.dll"
    INTERFACE_LINK_LIBRARIES slang::slang
)

add_library(slang_gfx SHARED IMPORTED GLOBAL)
add_library(slang::gfx ALIAS slang_gfx)
set_target_properties(slang_gfx PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SLANG_ROOT}/include"
    IMPORTED_IMPLIB "${SLANG_ROOT}/lib/gfx.lib"
    IMPORTED_LOCATION "${SLANG_ROOT}/bin/gfx.dll"
    INTERFACE_LINK_LIBRARIES slang::rt
)

if(TARGET ${CURRENT_MODULE_TARGET})
    target_link_libraries(${CURRENT_MODULE_TARGET} PUBLIC slang::gfx)

    set(ITEMS_TO_COPY "")

    list(APPEND ITEMS_TO_COPY slang::gfx slang::rt slang::slang)

    file(GLOB ALL_SLANG_DLLS "${SLANG_ROOT}/bin/*.dll")
    list(APPEND ITEMS_TO_COPY ${ALL_SLANG_DLLS})

    list(REMOVE_DUPLICATES ITEMS_TO_COPY)
    set(MODULE_DLL_COPY_LIST ${ITEMS_TO_COPY} PARENT_SCOPE)

endif()

april_helper_add_library(meshoptimizer ${CURRENT_MODULE_TARGET})
