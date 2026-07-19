
if(EMSCRIPTEN)
    FetchContent_Declare(
        emscripten_browser_clipboard
        GIT_REPOSITORY https://github.com/Armchair-Software/emscripten-browser-clipboard.git
        GIT_TAG        master
    )

    FetchContent_MakeAvailable(emscripten_browser_clipboard)

    target_include_directories(YourTarget PRIVATE
        ${emscripten_browser_clipboard_SOURCE_DIR}
    )

    target_link_options(YourTarget PRIVATE
        -sASYNCIFY
        "-sASYNCIFY_IMPORTS=[\"copy\",\"paste\"]"
        -sALLOW_MEMORY_GROWTH=1
    )
endif()
