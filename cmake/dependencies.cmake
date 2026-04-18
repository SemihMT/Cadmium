include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-3.4.4
)

FetchContent_MakeAvailable(SDL3)

if(CADMIUM_IMGUI)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.7-docking
)
FetchContent_MakeAvailable(imgui)
include(cmake/imgui.cmake)
endif()

if(NOT EMSCRIPTEN)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.14.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()
