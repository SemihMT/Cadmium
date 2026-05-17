// engine/src/scripting/script_host.cpp
#include <cadmium/scripting/script_host.hpp>
#include <cadmium/scripting/lua_bindings.hpp>
#include <cadmium/scripting/lua_bindings_assets.hpp>
#include <cadmium/scripting/lua_bindings_entity.hpp>
#include <cadmium/scripting/lua_bindings_components.hpp>
#include <cadmium/core/logger.hpp>

namespace Cadmium
{

ScriptHost::ScriptHost()
{
    m_Lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string,
        sol::lib::io,
        sol::lib::os,
        sol::lib::package);
}

void ScriptHost::BindAPIs(World&                  world,
                           AssetManager&           assets,
                           DrawCommandQueue&        queue,
                           InputManager&            input,
                           Lua::SceneBindingState&  sceneState)
{
    m_SceneState = &sceneState;
    m_World = &world;

    // Math, color, input, draw, scene table
    Lua::BindAll(m_Lua, input, queue, sceneState);

    // Asset bindings (LoadTexture, LoadFont, IsValid)
    Lua::BindAssets(m_Lua, assets);

    // Entity and component types. registered once per sol::state
    Lua::RegisterEntityTypes(m_Lua);
    Lua::RegisterComponentTypes(m_Lua);

    // Entity.New / Entity.Destroy / Component.*  bound to this host's world
    Lua::BindEntity(m_Lua, m_Registry, world.GetRegistry());
    Lua::BindComponents(m_Lua, world);
}

bool ScriptHost::Load(const std::string& source, const std::string& debugName)
{
    m_LastDebugName = debugName;
    ResetEnv();
    return Execute(source, debugName);
}

bool ScriptHost::Reload(const std::string& source, const std::string& debugName)
{
     m_LastDebugName = debugName;

    sol::protected_function onDestroy = m_Env["OnDestroy"];
    if (onDestroy.valid()) onDestroy();

    if (m_World)
        m_Registry.Clear(*m_World);

    ResetEnv();
    return Execute(source, debugName);
}

bool ScriptHost::Reload(const std::string& source)
{
    return Reload(source, m_LastDebugName);
}

void ScriptHost::ResetEnv()
{
    m_Env = sol::environment(m_Lua, sol::create, m_Lua.globals());
}

bool ScriptHost::Execute(const std::string& source, const std::string& debugName)
{
    auto result = m_Lua.safe_script(
        source,
        m_Env,
        sol::script_pass_on_error,
        "@" + debugName);

    if (!result.valid())
    {
        sol::error err = result;
        Log::Error("ScriptHost", "{}", err.what());
        return false;
    }

    sol::protected_function onEnter = m_Env["OnEnter"];
    if (onEnter.valid())
    {
        auto r = onEnter();
        if (!r.valid())
        {
            sol::error err = r;
            Log::Error("ScriptHost", "OnEnter error: {}", err.what());
            return false;
        }
    }

    return true;
}

} // namespace Cadmium
