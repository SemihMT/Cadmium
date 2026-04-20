#pragma once

#include <cadmium/core/scene.hpp>
#include "game_state.hpp"
#include "debris_system.hpp"
#include "menu_events.hpp"

namespace Sandbox
{
  class MenuScene;

  class AsteroidsScene : public Cadmium::Scene
  {
  public:
    AsteroidsScene() : Cadmium::Scene("Asteroids") {}
    void OnEnter()   override;
    void OnDestroy() override;

  private:
    Cadmium::SubscriptionToken m_ReturnToken;
  };

} // namespace Sandbox
