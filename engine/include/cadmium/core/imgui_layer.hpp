#ifndef CADMIUM_IMGUI_LAYER_HPP
#define CADMIUM_IMGUI_LAYER_HPP

#include <SDL3/SDL.h>
#include <webgpu/webgpu.h>
#include <cadmium/render/renderer_backend.hpp>

namespace Cadmium
{
  class IRenderer;
  class ImGuiLayer
  {
  public:
    void InitForSDLRenderer(SDL_Window* window, SDL_Renderer* renderer, IRenderer& backend);
    void InitForWebGPU(SDL_Window* window, WGPUDevice device, WGPUTextureFormat surfaceFormat, IRenderer& backend);
    void Shutdown();
    void ProcessEvent(SDL_Event& event);
    private:
    void Begin();
    void RenderDrawData(void* backendHandle);

    RendererBackend m_Backend{RendererBackend::SDL2D};
    SDL_Renderer* m_SDLRenderer{nullptr};
  };
} // namespace Cadmium

#endif // CADMIUM_IMGUI_LAYER_HPP
