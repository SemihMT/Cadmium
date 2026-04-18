add_library(cadmium-engine STATIC
    src/core/engine.cpp
    src/core/timer.cpp
)

target_include_directories(cadmium-engine
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cadmium-engine PUBLIC
    SDL3::SDL3
)

if(EMSCRIPTEN)
    target_compile_definitions(cadmium-engine PUBLIC CADMIUM_PLATFORM_WEB)
endif()
