#include "stress_scene.hpp"
#include "menu_events.hpp"
#include "menu_scene.hpp"
#include "movement_system.hpp"
#include "stress_debug_overlay.hpp"
#include "stress_render_layer.hpp"

namespace Sandbox
{
  void StressScene::OnEnter()
  {
    SetDefaultBackground(true);
    auto& system = RegisterSystem<MovementSystem>(0);
    system.SetBounds(GetWidth(), GetHeight());

    m_ReturnToken = Subscribe<ReturnToMenuEvent>([this](const ReturnToMenuEvent&)
    {
      ReplaceScene(std::make_unique<MenuScene>());
    });

    PushLayer(std::make_unique<StressRenderLayer>());
    PushOverlay(std::make_unique<StressDebugOverlay>());
  }

  void StressScene::OnDestroy()
  {
    m_ReturnToken = {};
  }

} // namespace Sandbox
