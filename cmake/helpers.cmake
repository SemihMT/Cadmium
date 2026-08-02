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

function(copy_assets_tracked source_dir target label)
    file(GLOB_RECURSE asset_files CONFIGURE_DEPENDS "${source_dir}/*")

    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/${target}_${label}_assets.stamp")

    add_custom_command(
        OUTPUT ${stamp}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${source_dir}
            $<TARGET_FILE_DIR:${target}>/assets
        COMMAND ${CMAKE_COMMAND} -E touch ${stamp}
        DEPENDS ${asset_files}
        COMMENT "Syncing ${label} assets for ${target}"
    )

    add_custom_target(${target}_${label}_assets_sync DEPENDS ${stamp})
    add_dependencies(${target} ${target}_${label}_assets_sync)
endfunction()
