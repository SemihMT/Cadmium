#ifndef CADMIUM_RENDERER_BACKEND_HPP
#define CADMIUM_RENDERER_BACKEND_HPP

namespace Cadmium
{
    enum class RendererBackend
    {
        SDL2D,  // SDLRenderer (the basic non-3d one)
        WebGPU, // will become the default between web and native once it's up and running
    };

    // Keeps the SDL2D as default for native for now
    constexpr RendererBackend DefaultRendererBackend()
    {
#ifdef CADMIUM_PLATFORM_WEB
        return RendererBackend::WebGPU;
#else
        return RendererBackend::SDL2D;
#endif
    }
} // namespace Cadmium
#endif // CADMIUM_RENDERER_BACKEND_HPP
