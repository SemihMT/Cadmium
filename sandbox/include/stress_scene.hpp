#pragma once

#include <cadmium/core/scene.hpp>
#include "movement_system.hpp"
#include "stress_render_layer.hpp"
#include "stress_debug_overlay.hpp"
#include "menu_events.hpp"

namespace Sandbox
{
  class MenuScene; // forward declaration

  class StressScene : public Cadmium::Scene
  {
  public:
    StressScene() : Cadmium::Scene("StressTest") {}
    void OnEnter()   override;
    void OnDestroy() override;

  private:
    Cadmium::SubscriptionToken m_ReturnToken;
  };

} // namespace Sandbox
