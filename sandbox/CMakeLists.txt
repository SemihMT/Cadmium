add_executable(cadmium-sandbox
    src/main.cpp
)

target_link_libraries(cadmium-sandbox PRIVATE
    cadmium-engine
)

copy_sdl_to_output(cadmium-sandbox)

if(EMSCRIPTEN)
    set_target_properties(cadmium-sandbox PROPERTIES
        SUFFIX ".html"
    )
    target_link_options(cadmium-sandbox PRIVATE
        -sUSE_SDL=0
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=1
    )
endif()
