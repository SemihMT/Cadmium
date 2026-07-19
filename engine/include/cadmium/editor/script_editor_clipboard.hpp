#ifndef CADMIUM_EDITOR_SCRIPT_EDITOR_CLIPBOARD_HPP
#define CADMIUM_EDITOR_SCRIPT_EDITOR_CLIPBOARD_HPP

//
//  ScriptEditorClipboard.hpp
//
//  Call SetupWebClipboard() once after ImGui has been fully initialised
//  (i.e. after ImGui_ImplSDL3_Init and ImGui_ImplOpenGL3_Init).
//
//  On non-Emscripten platforms this header is entirely inert — the SDL3
//  backend already wires up the native clipboard for ImGui automatically.
//
//  Requires (Emscripten only):
//    - emscripten-browser-clipboard (header-only, one file):
//        https://github.com/Armchair-Software/emscripten-browser-clipboard
//    - The following flags added to your Emscripten link options in CMake:
//
//        target_link_options(<YourTarget> PRIVATE
//            -sASYNCIFY
//            -sASYNCIFY_IMPORTS=["copy","paste"]
//            -sALLOW_MEMORY_GROWTH=1
//        )
//
//  Usage:
//
//      // App init, after all ImGui backend Init calls:
//      Cadmium::Editor::SetupWebClipboard();
//
//      // Then your normal render loop — nothing else to do.
//

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

#ifdef CADMIUM_PLATFORM_WEB
#include <emscripten_browser_clipboard.h>
#include <string>
#endif

namespace Cadmium::Editor
{

#ifdef CADMIUM_PLATFORM_WEB
namespace detail
{
    // Internal mirror of the browser clipboard contents.
    // Must be a long-lived object because ImGui holds a raw const char* to it
    // between the GetClipboardTextFn call and the end of the frame.
    inline std::string g_webClipboard;

    inline const char* WebGetClipboard(ImGuiContext* /*ctx*/)
    {
        return g_webClipboard.c_str();
    }

    inline void WebSetClipboard(ImGuiContext* /*ctx*/, const char* text)
    {
        g_webClipboard = text ? text : "";
        emscripten_browser_clipboard::copy(g_webClipboard);
    }
} // namespace detail
#endif // CADMIUM_PLATFORM_WEB

inline void SetupWebClipboard()
{
#if defined(CADMIUM_PLATFORM_WEB) && defined(CADMIUM_IMGUI)
    // Register a passive paste listener.
    // The browser fires this whenever the user pastes into the page
    // (Ctrl+V / Cmd+V), filling our mirror so GetClipboardTextFn can
    // return it synchronously when ImGui asks for it.
    emscripten_browser_clipboard::paste(
        [](std::string const& data, void* /*userData*/)
        {
            detail::g_webClipboard = data;
        });

    ImGuiPlatformIO& pio            = ImGui::GetPlatformIO();
    pio.Platform_GetClipboardTextFn = detail::WebGetClipboard;
    pio.Platform_SetClipboardTextFn = detail::WebSetClipboard;
#endif
}

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_EDITOR_CLIPBOARD_HPP
