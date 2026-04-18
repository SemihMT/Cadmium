function(copy_sdl_to_output target)
    if(WIN32 AND NOT EMSCRIPTEN)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL3::SDL3>
                $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying SDL3 DLL to output directory"
        )
    endif()
endfunction()
