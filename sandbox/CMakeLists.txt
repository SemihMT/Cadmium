add_executable(cadmium-sandbox
    src/main.cpp
    src/asteroids_scene.cpp
    src/stress_scene.cpp
    src/menu_scene.cpp
    src/world_layer.cpp
    src/render_layer.cpp
    src/hud_layer.cpp
    src/debug_overlay.cpp
    src/stress_render_layer.cpp
    src/stress_debug_overlay.cpp
    src/menu_layer.cpp
)

target_include_directories(cadmium-sandbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cadmium-sandbox PRIVATE
    cadmium-engine
)

copy_sdl_to_output(cadmium-sandbox)
if(NOT EMSCRIPTEN)
    target_copy_webgpu_binaries(cadmium-sandbox)
endif()
copy_assets_tracked(${CMAKE_SOURCE_DIR}/engine/assets  cadmium-sandbox engine)
copy_assets_tracked(${CMAKE_SOURCE_DIR}/sandbox/assets cadmium-sandbox sandbox)

if(EMSCRIPTEN)
    file(GLOB_RECURSE engine_asset_files  CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/engine/assets/*")
    file(GLOB_RECURSE sandbox_asset_files CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/sandbox/assets/*")

    set_target_properties(cadmium-sandbox PROPERTIES
        SUFFIX ".html"
        LINK_DEPENDS "${engine_asset_files};${sandbox_asset_files}"
    )
    target_link_options(cadmium-sandbox PRIVATE
        -sUSE_SDL=0
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=1
        -sNO_DISABLE_EXCEPTION_CATCHING
        -sASYNCIFY
        "-sASYNCIFY_IMPORTS=[\"copy\",\"paste\"]"
        --emrun
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/engine/assets@/assets"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/sandbox/assets@/assets"
    )
endif()
