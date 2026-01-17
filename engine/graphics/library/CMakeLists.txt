add_subdirectory(slang-rhi EXCLUDE_FROM_ALL)

set(SLANG_TARGETS slang slang-rhi)
foreach(T ${SLANG_TARGETS})
    if(TARGET ${T})
        get_target_property(IS_IMPORTED ${T} IMPORTED)
        if(NOT IS_IMPORTED)
            target_compile_options(${T} PRIVATE /permissive /wd4996 /wd4596)
            target_compile_definitions(${T} PRIVATE
                _SILENCE_CXX20_IS_POD_DEPRECATED_WARNING
                _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
            )
        endif()

        get_target_property(_INC_DIRS ${T} INTERFACE_INCLUDE_DIRECTORIES)

        if(_INC_DIRS)
            set_target_properties(${T} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_INC_DIRS}"
            )
        endif()
    endif()
endforeach()

if(TARGET ${CURRENT_MODULE_TARGET})
    message(STATUS "[Library] Linking 'slang' and 'slang-rhi' to '${CURRENT_MODULE_TARGET}'")

    target_link_libraries(${CURRENT_MODULE_TARGET} PUBLIC
        slang
        slang-rhi
    )
endif()

set(_DLL_TARGETS slang slang-rhi)

set(MODULE_DLL_COPY_LIST ${_DLL_TARGETS} PARENT_SCOPE)

april_helper_add_library(meshoptimizer ${CURRENT_MODULE_TARGET})
