include(${CMAKE_CURRENT_LIST_DIR}/april-platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/april-manifest.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/april-resources.cmake)

function(april_resolve_dependency RAW_DEP OUT_VAR)
    if(TARGET "${RAW_DEP}")
        set(${OUT_VAR} "${RAW_DEP}" PARENT_SCOPE)
        return()
    endif()

    set(PREFIXED "April_${RAW_DEP}")
    if(TARGET "${PREFIXED}")
        set(${OUT_VAR} "${PREFIXED}" PARENT_SCOPE)
        return()
    endif()

    set(${OUT_VAR} "${RAW_DEP}" PARENT_SCOPE)
endfunction()

function(april_link_target_list MODULE_NAME VISIBILITY DEP_LIST)
    if(DEP_LIST STREQUAL "" OR DEP_LIST MATCHES "-NOTFOUND$")
        return()
    endif()

    set(LOCAL_DEPS "${DEP_LIST}")
    list(REMOVE_DUPLICATES LOCAL_DEPS)

    foreach(DEP IN LISTS LOCAL_DEPS)
        string(STRIP "${DEP}" DEP)
        if(DEP STREQUAL "")
            continue()
        endif()

        april_resolve_dependency("${DEP}" RESOLVED_DEP)

        if(RESOLVED_DEP STREQUAL "")
            continue()
        endif()

        if(NOT TARGET "${RESOLVED_DEP}" AND NOT EXISTS "${RESOLVED_DEP}")
            message(WARNING "[Link] '${MODULE_NAME}' dependency '${DEP}' not found as target or path. Passing to linker.")
        endif()

        message(STATUS "[Link Debug] Linking '${MODULE_NAME}' with '${RESOLVED_DEP}'")
        target_link_libraries(${MODULE_NAME} ${VISIBILITY} ${RESOLVED_DEP})
    endforeach()
endfunction()

function(april_apply_compile_definitions TARGET_NAME VISIBILITY DEFINES)
    if(DEFINES STREQUAL "" OR DEFINES MATCHES "-NOTFOUND$")
        return()
    endif()

    target_compile_definitions(${TARGET_NAME} ${VISIBILITY} ${DEFINES})
endfunction()

function(april_apply_platform_definitions TARGET_NAME BASE_PUBLIC BASE_PRIVATE WINDOWS_PUBLIC WINDOWS_PRIVATE LINUX_PUBLIC LINUX_PRIVATE MACOS_PUBLIC MACOS_PRIVATE)
    april_apply_compile_definitions(${TARGET_NAME} PUBLIC "${BASE_PUBLIC}")
    april_apply_compile_definitions(${TARGET_NAME} PRIVATE "${BASE_PRIVATE}")

    april_platform_select_list(PLATFORM_PUBLIC "${WINDOWS_PUBLIC}" "${LINUX_PUBLIC}" "${MACOS_PUBLIC}")
    april_platform_select_list(PLATFORM_PRIVATE "${WINDOWS_PRIVATE}" "${LINUX_PRIVATE}" "${MACOS_PRIVATE}")

    april_apply_compile_definitions(${TARGET_NAME} PUBLIC "${PLATFORM_PUBLIC}")
    april_apply_compile_definitions(${TARGET_NAME} PRIVATE "${PLATFORM_PRIVATE}")
endfunction()

function(april_link_platform_dependencies TARGET_NAME BASE_PUBLIC BASE_PRIVATE WINDOWS_PUBLIC WINDOWS_PRIVATE LINUX_PUBLIC LINUX_PRIVATE MACOS_PUBLIC MACOS_PRIVATE)
    april_link_target_list(${TARGET_NAME} PUBLIC "${BASE_PUBLIC}")
    april_link_target_list(${TARGET_NAME} PRIVATE "${BASE_PRIVATE}")

    april_platform_select_list(PLATFORM_PUBLIC "${WINDOWS_PUBLIC}" "${LINUX_PUBLIC}" "${MACOS_PUBLIC}")
    april_platform_select_list(PLATFORM_PRIVATE "${WINDOWS_PRIVATE}" "${LINUX_PRIVATE}" "${MACOS_PRIVATE}")

    april_link_target_list(${TARGET_NAME} PUBLIC "${PLATFORM_PUBLIC}")
    april_link_target_list(${TARGET_NAME} PRIVATE "${PLATFORM_PRIVATE}")
endfunction()

function(april_helper_add_codegen MODULE_NAME MODULE_PATH MANIFEST_PATH CMD OUTPUT_DIR)
    if(CMD STREQUAL "")
        return()
    endif()

    set(CODEGEN_DIR "${CMAKE_BINARY_DIR}/codegen/${MODULE_NAME}")
    set(STAMP_FILE "${CODEGEN_DIR}/codegen.stamp")

    set(CMD_LIST "")
    separate_arguments(CMD_LIST NATIVE_COMMAND "${CMD}")

    # Build dependency set so codegen reruns when inputs/generator change.
    set(CODEGEN_DEPENDS "${MANIFEST_PATH}")

    list(LENGTH CMD_LIST CMD_LIST_LEN)
    if(CMD_LIST_LEN GREATER 0)
        list(GET CMD_LIST 0 CMD_EXE)
        if(CMD_EXE MATCHES "slang-codegen\\.py$")
            if(IS_ABSOLUTE "${CMD_EXE}")
                set(CODEGEN_SCRIPT_PATH "${CMD_EXE}")
            else()
                set(CODEGEN_SCRIPT_PATH "${MODULE_PATH}/${CMD_EXE}")
            endif()
            if(EXISTS "${CODEGEN_SCRIPT_PATH}")
                list(APPEND CODEGEN_DEPENDS "${CODEGEN_SCRIPT_PATH}")
            endif()
        endif()
    endif()

    if(CMD_LIST_LEN GREATER 1)
        list(GET CMD_LIST 0 CMD_EXE)
        if(CMD_EXE STREQUAL "python" OR CMD_EXE STREQUAL "python3")
            list(GET CMD_LIST 1 CODEGEN_SCRIPT_ARG)
            if(IS_ABSOLUTE "${CODEGEN_SCRIPT_ARG}")
                set(CODEGEN_SCRIPT_PATH "${CODEGEN_SCRIPT_ARG}")
            else()
                set(CODEGEN_SCRIPT_PATH "${MODULE_PATH}/${CODEGEN_SCRIPT_ARG}")
            endif()
            if(EXISTS "${CODEGEN_SCRIPT_PATH}")
                list(APPEND CODEGEN_DEPENDS "${CODEGEN_SCRIPT_PATH}")
            endif()
        endif()
    endif()

    if(CMD_LIST_LEN GREATER 1)
        math(EXPR CODEGEN_LAST_ARG_INDEX "${CMD_LIST_LEN} - 2")
        foreach(ARG_INDEX RANGE 0 ${CODEGEN_LAST_ARG_INDEX})
            list(GET CMD_LIST ${ARG_INDEX} ARG_KEY)
            math(EXPR ARG_VALUE_INDEX "${ARG_INDEX} + 1")
            list(GET CMD_LIST ${ARG_VALUE_INDEX} ARG_VALUE)

            if(ARG_KEY STREQUAL "--input")
                if(IS_ABSOLUTE "${ARG_VALUE}")
                    set(CODEGEN_INPUT_FILE "${ARG_VALUE}")
                else()
                    set(CODEGEN_INPUT_FILE "${MODULE_PATH}/${ARG_VALUE}")
                endif()
                if(EXISTS "${CODEGEN_INPUT_FILE}")
                    list(APPEND CODEGEN_DEPENDS "${CODEGEN_INPUT_FILE}")
                endif()
            elseif(ARG_KEY STREQUAL "--input-dir")
                if(IS_ABSOLUTE "${ARG_VALUE}")
                    set(CODEGEN_INPUT_DIR "${ARG_VALUE}")
                else()
                    set(CODEGEN_INPUT_DIR "${MODULE_PATH}/${ARG_VALUE}")
                endif()
                if(IS_DIRECTORY "${CODEGEN_INPUT_DIR}")
                    file(GLOB_RECURSE CODEGEN_INPUT_FILES CONFIGURE_DEPENDS "${CODEGEN_INPUT_DIR}/*")
                    list(APPEND CODEGEN_DEPENDS ${CODEGEN_INPUT_FILES})
                endif()
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES CODEGEN_DEPENDS)

    add_custom_command(
        OUTPUT "${STAMP_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CODEGEN_DIR}"
        COMMAND ${CMD_LIST}
        COMMAND ${CMAKE_COMMAND} -E touch "${STAMP_FILE}"
        WORKING_DIRECTORY "${MODULE_PATH}"
        DEPENDS ${CODEGEN_DEPENDS}
        COMMENT "[${MODULE_NAME}] Running codegen"
    )

    add_custom_target(${MODULE_NAME}_codegen DEPENDS "${STAMP_FILE}")
    add_dependencies(${MODULE_NAME} ${MODULE_NAME}_codegen)

    if(NOT OUTPUT_DIR STREQUAL "")
        if(NOT IS_ABSOLUTE "${OUTPUT_DIR}")
            set(OUTPUT_DIR "${MODULE_PATH}/${OUTPUT_DIR}")
        endif()

        target_include_directories(${MODULE_NAME} PRIVATE "${OUTPUT_DIR}")
    endif()
endfunction()

function(april_helper_handle_library MODULE_NAME MODULE_PATH DIR_NAME)
    if(NOT EXISTS "${MODULE_PATH}/library/CMakeLists.txt")
        return()
    endif()

    set(CURRENT_MODULE_TARGET ${MODULE_NAME})
    set(MODULE_DLL_COPY_LIST "")

    add_subdirectory("${MODULE_PATH}/library" "${CMAKE_BINARY_DIR}/bin/lib/${MODULE_NAME}")

    foreach(ITEM ${MODULE_DLL_COPY_LIST})
        set(FILE_TO_COPY "")

        if(TARGET ${ITEM})
            get_target_property(TARGET_TYPE ${ITEM} TYPE)
            if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY" OR "${TARGET_TYPE}" STREQUAL "UNKNOWN_LIBRARY")
                get_target_property(IMPORTED_LOC ${ITEM} IMPORTED_LOCATION)
                if(IMPORTED_LOC)
                    set(FILE_TO_COPY "${IMPORTED_LOC}")
                else()
                    set(FILE_TO_COPY "$<TARGET_FILE:${ITEM}>")
                endif()
            endif()

        elseif(EXISTS "${ITEM}")
            set(FILE_TO_COPY "${ITEM}")
        endif()

        if(FILE_TO_COPY)
            get_filename_component(ITEM_NAME "${FILE_TO_COPY}" NAME)

            add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${FILE_TO_COPY}"
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                COMMENT "Copying dependency: ${ITEM_NAME} -> bin/"
            )
        endif()
    endforeach()
endfunction()

function(april_helper_add_library LIBRARY_NAME MODULE_TARGET)
    add_subdirectory(${LIBRARY_NAME} EXCLUDE_FROM_ALL)

    if(TARGET ${MODULE_TARGET})
        message(STATUS "[Library] Linking '${LIBRARY_NAME}' to '${MODULE_TARGET}'")
        target_link_libraries(${MODULE_TARGET} PUBLIC ${LIBRARY_NAME})
    endif()
endfunction()
