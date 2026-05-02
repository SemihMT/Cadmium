#include "cadmium/scripting/scripted_scene.hpp"
#include "cadmium/scripting/script_render_layer.hpp"
#include "cadmium/scripting/script_update_layer.hpp"
#include "cadmium/core/engine_context.hpp"

namespace Cadmium
{

void ScriptedScene::OnEnter()
{
    PushLayer(std::make_unique<ScriptUpdateLayer>(m_Env));
    // ScriptRenderLayer always present — scripts can always draw
    PushLayer(std::make_unique<ScriptRenderLayer>(GetDrawQueue(), GetFont()));

    // Load and execute the script
    // The script runs immediately — this is where it defines its globals
    // and sets up any entities or state it needs
    auto& lua = GetLua();

    // Create an isolated environment that falls back to the global
    // environment for builtins (math, string, Draw, Input etc)
    m_Env = sol::environment(lua, sol::create, lua.globals());

    // Load the script into this environment — not into global scope
    auto result = lua.safe_script_file(
        m_ScriptPath,
        m_Env,
        sol::script_pass_on_error
    );

    if (!result.valid())
    {
        sol::error err = result;
        SDL_Log("[ScriptedScene] Failed to load '%s': %s",
                m_ScriptPath.c_str(), err.what());
        return;
    }

    // Hooks are now in the environment, not in globals
    sol::protected_function onEnter = m_Env["OnEnter"];
    if (onEnter.valid()) onEnter();
}

void ScriptedScene::OnExit()
{
    auto& lua = GetLua();
    sol::protected_function onExit = lua["OnExit"];
    if (onExit.valid()) onExit();
}

void ScriptedScene::OnDestroy()
{
    sol::protected_function onDestroy = m_Env["OnDestroy"];
    if (onDestroy.valid()) onDestroy();

    // Environment is destroyed with the scene — no manual cleanup needed
    m_Env = sol::environment{};
}

} // namespace Cadmium
