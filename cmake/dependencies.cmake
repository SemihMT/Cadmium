include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-3.4.4
)

FetchContent_MakeAvailable(SDL3)

set(SDLTTF_VENDORED ON)
FetchContent_Declare(
    SDL_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
)

FetchContent_MakeAvailable(SDL_ttf)

FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.4.2
)

FetchContent_MakeAvailable(SDL_image)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)

#fetch Lua
include(cmake/lua.cmake)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.5.0
)
FetchContent_MakeAvailable(sol2)

if(CADMIUM_IMGUI)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.7-docking
)
FetchContent_MakeAvailable(imgui)
include(cmake/imgui.cmake)

FetchContent_Declare(
    colortextedit
    GIT_REPOSITORY https://github.com/goossens/ImGuiColorTextEdit.git
    GIT_TAG a74fb090d2ea9276ae6c35c2f6ab39491c7d404f
)
FetchContent_MakeAvailable(colortextedit)
include(cmake/colortextedit.cmake)

FetchContent_Declare(
    emscripten_browser_clipboard
    GIT_REPOSITORY https://github.com/Armchair-Software/emscripten-browser-clipboard.git
    GIT_TAG        5a962a041a11bf2f5948a5b9f5470eee4476274d
)
FetchContent_MakeAvailable(emscripten_browser_clipboard)

endif()

if(NOT EMSCRIPTEN)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.14.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()
