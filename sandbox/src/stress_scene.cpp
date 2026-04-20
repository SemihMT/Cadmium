#include "stress_scene.hpp"
#include "menu_scene.hpp" // complete type needed for make_unique

namespace Sandbox
{
  void StressScene::OnEnter()
  {
    SetDefaultBackground(false);
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
