function(april_resource_queue_copy MODULE_NAME TARGET_NAME SRC_DIR DST_DIR TAG)
    if(NOT TARGET ${TARGET_NAME})
        message(WARNING "[${MODULE_NAME}] Resource sync skipped: target '${TARGET_NAME}' not found.")
        return()
    endif()

    if(NOT EXISTS "${SRC_DIR}")
        message(WARNING "[${MODULE_NAME}] Resource path not found: ${SRC_DIR}")
        return()
    endif()

    message(STATUS "[${MODULE_NAME}] Queued ${TAG} sync: ${SRC_DIR} -> ${DST_DIR}")

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_DIR}" "${DST_DIR}"
        COMMENT "[${MODULE_NAME}] Syncing ${TAG}: ${SRC_DIR} -> ${DST_DIR}"
    )
endfunction()

function(april_helper_sync_resources MODULE_NAME MODULE_PATH COPY_DIR_LIST)
    if(COPY_DIR_LIST STREQUAL "" OR COPY_DIR_LIST MATCHES "-NOTFOUND$")
        return()
    endif()

    foreach(PAIR IN LISTS COPY_DIR_LIST)
        string(REPLACE "|" ";" PARTS "${PAIR}")
        list(LENGTH PARTS PARTS_LEN)
        if(NOT PARTS_LEN EQUAL 2)
            message(WARNING "[${MODULE_NAME}] Invalid resource pair '${PAIR}'. Expected 'src|dst'.")
            continue()
        endif()

        list(GET PARTS 0 SRC_DIR)
        list(GET PARTS 1 DST_DIR)

        if(NOT IS_ABSOLUTE "${SRC_DIR}")
            set(SRC_DIR "${MODULE_PATH}/${SRC_DIR}")
        endif()

        if(NOT IS_ABSOLUTE "${DST_DIR}")
            set(DST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${DST_DIR}")
        endif()

        april_resource_queue_copy("${MODULE_NAME}" "${MODULE_NAME}" "${SRC_DIR}" "${DST_DIR}" "resource")
    endforeach()
endfunction()

function(april_helper_sync_shader MODULE_NAME MODULE_PATH DIR_NAME)
    set(SHADER_SRC "${MODULE_PATH}/shader")
    set(SHADER_DST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/${DIR_NAME}")

    if(NOT EXISTS "${SHADER_SRC}")
        message(STATUS "[${MODULE_NAME}] Shader directory not found. Skipping shader sync.")
        return()
    endif()

    april_resource_queue_copy("${MODULE_NAME}" "${MODULE_NAME}" "${SHADER_SRC}" "${SHADER_DST}" "shader")
endfunction()
