function(april_helper_sync_shader DIR_NAME PATH)
    set(MODULE_NAME "April_${DIR_NAME}")
    set(SHADER_SRC "${PATH}/shader")
    set(SHADER_DST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/${DIR_NAME}")

    if(EXISTS "${SHADER_SRC}" AND TARGET ${MODULE_NAME})
        add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_DST}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_SRC}" "${SHADER_DST}"
            COMMENT "[${MODULE_NAME}] Syncing shaders to bin/..."
        )
    endif()
endfunction()

function(april_parse_manifest FILE_PATH OUT_VERSION_VAR OUT_DEPS_VAR)
    if(NOT EXISTS "${FILE_PATH}")
        return()
    endif()

    file(STRINGS "${FILE_PATH}" LINES)

    set(VERSION "0.0.0")
    set(DEPS "")

    foreach(LINE ${LINES})
        string(STRIP "${LINE}" LINE)

        if(LINE STREQUAL "" OR LINE MATCHES "^#")
            continue()
        endif()

        if(LINE MATCHES "^VERSION:[ \t]*(.*)$")
            set(VERSION "${CMAKE_MATCH_1}")
        else()
            list(APPEND DEPS "April_${LINE}")
        endif()
    endforeach()

    set(${OUT_VERSION_VAR} "${VERSION}" PARENT_SCOPE)
    set(${OUT_DEPS_VAR} "${DEPS}" PARENT_SCOPE)
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
