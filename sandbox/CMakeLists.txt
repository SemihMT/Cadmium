add_executable(cadmium-sandbox
    src/main.cpp
    src/world_layer.cpp
    src/render_layer.cpp
    src/hud_layer.cpp
    src/debug_overlay.cpp
)

target_include_directories(cadmium-sandbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cadmium-sandbox PRIVATE
    cadmium-engine
)

copy_sdl_to_output(cadmium-sandbox)
copy_engine_assets(cadmium-sandbox)

if(EMSCRIPTEN)
    set_target_properties(cadmium-sandbox PROPERTIES
        SUFFIX ".html"
    )
    target_link_options(cadmium-sandbox PRIVATE
        -sUSE_SDL=0
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=1
        --emrun
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/engine/assets@/assets"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/sandbox/assets@/assets"
    )
endif()
