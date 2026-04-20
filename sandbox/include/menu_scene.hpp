#pragma once

#include <cadmium/core/scene.hpp>

namespace Sandbox
{
  class MenuLayer;
  class MenuScene : public Cadmium::Scene
  {
  public:
    MenuScene() : Cadmium::Scene("Menu") {}
    void OnEnter() override;
  };

} // namespace Sandbox
