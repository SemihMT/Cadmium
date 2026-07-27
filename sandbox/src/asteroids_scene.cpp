#include "asteroids_scene.hpp"
#include "cadmium/ecs/components.hpp"
#include "debris_system.hpp"
#include "menu_events.hpp"
#include "menu_scene.hpp"
#include "world_layer.hpp"
#include <cadmium/core/assets.hpp>
#include <sol/types.hpp>
#include "render_layer.hpp"
#include "hud_layer.hpp"
#include "debug_overlay.hpp"
#include <cadmium/scripting/script_system.hpp>

namespace Sandbox
{
  void AsteroidsScene::OnEnter()
  {
    SetDefaultBackground(false);

    Cadmium::Entity player = CreateEntity();
    GetWorld().AddComponent<Cadmium::Tag>(player, {"Player"});
    AttachScript(player, "assets/scripts/test_script.lua");

    Cadmium::Entity enemy = CreateEntity();
    AttachScript(enemy, "assets/scripts/enemy_ai.lua");
    AttachScript(enemy, "assets/scripts/health.lua");

    RegisterSystem<Cadmium::ScriptSystem>(0);
    RegisterSystem<DebrisSystem>(1);


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
