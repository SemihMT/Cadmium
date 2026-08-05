#include <cadmium/render/native_surface.hpp>
#include <cadmium/core/logger.hpp>

#if defined(_WIN32)
    #define CADMIUM_WGPU_WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #define CADMIUM_WGPU_COCOA
#elif defined(__linux__)
    #include <SDL3/SDL_video.h>
    // Decided at runtime below: X11 vs Wayland
#endif

namespace Cadmium
{
    WGPUSurface CreateWGPUSurfaceFromSDLWindow(WGPUInstance instance, SDL_Window* window)
    {
        SDL_PropertiesID props = SDL_GetWindowProperties(window);

#if defined(CADMIUM_WGPU_WIN32)
        HWND hwnd = static_cast<HWND>(
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        HINSTANCE hinstance = static_cast<HINSTANCE>(
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr));

        WGPUSurfaceSourceWindowsHWND source{};
        source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
        source.hinstance = hinstance;
        source.hwnd = hwnd;

        WGPUSurfaceDescriptor desc{};
        desc.nextInChain = &source.chain;
        desc.label = {"Cadmium Surface", WGPU_STRLEN};
        return wgpuInstanceCreateSurface(instance, &desc);

#elif defined(CADMIUM_WGPU_COCOA)
        // NSWindow -> CAMetalLayer requires the small ObjC helper below
        // (native_surface_cocoa.mm) - SDL only hands you the NSWindow*,
        // not a Metal layer.
        void* metalLayer = Cadmium_CreateMetalLayerForWindow(
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr));

        WGPUSurfaceSourceMetalLayer source{};
        source.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        source.layer = metalLayer;

        WGPUSurfaceDescriptor desc{};
        desc.nextInChain = &source.chain;
        desc.label = {"Cadmium Surface", WGPU_STRLEN};
        return wgpuInstanceCreateSurface(instance, &desc);

#elif defined(__linux__)
        const char* videoDriver = SDL_GetCurrentVideoDriver();

        if (videoDriver && std::string_view(videoDriver) == "wayland")
        {
            void* display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
            void* surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

            WGPUSurfaceSourceWaylandSurface source{};
            source.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
            source.display = display;
            source.surface = surface;

            WGPUSurfaceDescriptor desc{};
            desc.nextInChain = &source.chain;
            desc.label = {"Cadmium Surface", WGPU_STRLEN};
            return wgpuInstanceCreateSurface(instance, &desc);
        }
        else // X11
        {
            void* display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
            Sint64 xwindow = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

            WGPUSurfaceSourceXlibWindow source{};
            source.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
            source.display = display;
            source.window = static_cast<uint64_t>(xwindow);

            WGPUSurfaceDescriptor desc{};
            desc.nextInChain = &source.chain;
            desc.label = {"Cadmium Surface", WGPU_STRLEN};
            return wgpuInstanceCreateSurface(instance, &desc);
        }
#else
        Cadmium::Log::Error("NativeSurface", "Unsupported platform for WGPU surface creation");
        return nullptr;
#endif
    }
}
