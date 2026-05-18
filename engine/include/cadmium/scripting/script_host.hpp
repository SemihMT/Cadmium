#ifndef CADMIUM_SCRIPTING_SCRIPT_HOST_HPP
#define CADMIUM_SCRIPTING_SCRIPT_HOST_HPP

#include <cadmium/scripting/entity_registry.hpp>
#include <cadmium/scripting/script_controller.hpp>
#include <cadmium/scripting/lua_bindings.hpp>
#include <sol/sol.hpp>
#include <string>

namespace Cadmium
{

class World;
class AssetManager;
class DrawCommandQueue;
class InputManager;

class ScriptHost : public IScriptController
{
public:
    ScriptHost();

    void BindAPIs(World&             world,
                  AssetManager&      assets,
                  DrawCommandQueue&  queue,
                  InputManager&      input,
                  Lua::SceneBindingState& sceneState);
    void UpdateSceneState();

    // IScriptController
    bool Reload(const std::string& source) override;
    void Pause(bool paused) override { m_Paused = paused; }

    bool Load(const std::string& source, const std::string& debugName);
    bool Reload(const std::string& source, const std::string& debugName);

    bool IsPaused()         const { return m_Paused; }

    sol::state&       GetLua()      { return m_Lua; }
    sol::environment& GetEnv()      { return m_Env; }
    EntityRegistry&   GetRegistry() { return m_Registry; }

private:
    bool Execute(const std::string& source, const std::string& debugName);
    void ResetEnv();

    sol::state              m_Lua;
    sol::environment        m_Env{};
    EntityRegistry          m_Registry;
    World *m_World{nullptr};
    bool                    m_Paused{false};
    std::string             m_LastDebugName;

    Lua::SceneBindingState* m_SceneState{nullptr};
};

} // namespace Cadmium

#endif // CADMIUM_SCRIPTING_SCRIPT_HOST_HPP
