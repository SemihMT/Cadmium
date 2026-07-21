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

    m_Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
    m_Lua.script_file("assets/scripts/test_script.lua");

    auto entity = GetWorld().CreateEntity();
    Cadmium::Script script;
    script.self = m_Lua.create_table();
    script.onUpdate = m_Lua["OnUpdate"];
    GetWorld().AddComponent<Cadmium::Script>(entity,script);

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
