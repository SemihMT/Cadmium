#ifndef CADMIUM_RENDER_NATIVE_SURFACE_HPP
#define CADMIUM_RENDER_NATIVE_SURFACE_HPP

#if defined(CADMIUM_WGPU_COCOA)
extern "C" void* Cadmium_CreateMetalLayerForWindow(void* nsWindowPtr);
#endif

#include <webgpu/webgpu.h>
#include <SDL3/SDL.h>

namespace Cadmium
{
    WGPUSurface CreateWGPUSurfaceFromSDLWindow(WGPUInstance instance, SDL_Window* window);
}
#endif
