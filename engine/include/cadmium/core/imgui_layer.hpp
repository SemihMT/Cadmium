#ifndef CADMIUM_IMGUI_LAYER_HPP
#define CADMIUM_IMGUI_LAYER_HPP

#include <SDL3/SDL.h>

namespace Cadmium
{
  class ImGuiLayer
  {
  public:
    void Init(SDL_Window* window, SDL_Renderer* renderer);
    void Shutdown();
    void Begin();
    void End(SDL_Renderer* renderer);
    void ProcessEvent(SDL_Event& event);
  };
} // namespace Cadmium

#endif // CADMIUM_IMGUI_LAYER_HPP
