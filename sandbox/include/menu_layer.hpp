#pragma once

#include <cadmium/core/layer.hpp>
#include <SDL3/SDL.h>
#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Sandbox
{
  class AsteroidsScene;
  class StressScene;

  class MenuLayer : public Cadmium::Layer
  {
  public:
    MenuLayer() : Cadmium::Layer("Menu") {}
    void OnImGuiRender() override;
    void OnEvent(SDL_Event& event) override;
  };

} // namespace Sandbox
