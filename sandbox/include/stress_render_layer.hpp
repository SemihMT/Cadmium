#pragma once

#include <cadmium/core/layer.hpp>
#include <SDL3/SDL.h>

namespace Sandbox
{
  class StressRenderLayer : public Cadmium::Layer
  {
  public:
    StressRenderLayer() : Cadmium::Layer("StressRender") {}
    void OnRender(SDL_Renderer* renderer) override;
  };

} // namespace Sandbox
