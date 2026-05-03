#include "cadmium/scripting/scripted_scene.hpp"
#include "cadmium/scripting/script_render_layer.hpp"
#include "cadmium/scripting/script_update_layer.hpp"
#include "cadmium/scripting/lua_bindings_entity.hpp"
#include "cadmium/core/engine_context.hpp"

namespace Cadmium
{

  void ScriptedScene::OnEnter()
  {
    auto &lua = GetLua();
    auto &ecsReg = GetWorld().GetRegistry();

    // Create isolated environment so script globals don't leak between scenes
    m_Env = sol::environment(lua, sol::create, lua.globals());

    // Bind Entity API into this scene's environment
    // Each scene gets its own EntityRegistry so entities don't leak
    Lua::BindEntity(lua, m_EntityRegistry, ecsReg);

    // Push layers - order matters
    // ScriptUpdateLayer calls OnUpdate/OnRender hooks
    // ScriptRenderLayer drains the DrawCommandQueue
    PushLayer(std::make_unique<ScriptUpdateLayer>(m_Env, m_EntityRegistry));
    PushLayer(std::make_unique<ScriptRenderLayer>(GetDrawQueue(), GetFont()));

    // Load and execute the script into the isolated environment
    auto result = lua.safe_script_file(
        m_ScriptPath,
        m_Env,
        sol::script_pass_on_error);

    if (!result.valid())
    {
      sol::error err = result;
      SDL_Log("[ScriptedScene] Failed to load '%s': %s",
              m_ScriptPath.c_str(), err.what());
      return;
    }

    // Call OnEnter if the script defines it
    sol::protected_function onEnter = m_Env["OnEnter"];
    if (onEnter.valid())
    {
      auto r = onEnter();
      if (!r.valid())
      {
        sol::error err = r;
        SDL_Log("[ScriptedScene] OnEnter error: %s", err.what());
      }
    }
  }

  void ScriptedScene::OnExit()
  {
    sol::protected_function f = m_Env["OnExit"];
    if (f.valid())
      f();
  }

  void ScriptedScene::OnDestroy()
  {
    sol::protected_function f = m_Env["OnDestroy"];
    if (f.valid())
      f();

    // Clean up all remaining entities
    m_EntityRegistry.Clear();

    // Release the environment - all script globals are cleaned up
    m_Env = sol::environment{};
  }

} // namespace Cadmium
