#include "asteroids_scene.hpp"
#include "menu_scene.hpp"
#include "world_layer.hpp"
#include "render_layer.hpp"
#include "hud_layer.hpp"
#include "debug_overlay.hpp"

namespace Sandbox
{
  void AsteroidsScene::OnEnter()
  {
    RegisterSystem<DebrisSystem>(0);

    m_ReturnToken = Subscribe<ReturnToMenuEvent>([this](const ReturnToMenuEvent&)
    {
      ReplaceScene(std::make_unique<MenuScene>());
    });

    auto state = std::make_shared<GameState>();
    PushLayer(std::make_unique<WorldLayer>(state));
    PushLayer(std::make_unique<RenderLayer>(state));
    PushLayer(std::make_unique<HUDLayer>(state));
    PushOverlay(std::make_unique<DebugOverlay>(state));
  }

  void AsteroidsScene::OnDestroy()
  {
    m_ReturnToken = {};
  }

} // namespace Sandbox
