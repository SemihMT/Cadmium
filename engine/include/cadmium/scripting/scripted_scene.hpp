#ifndef CADMIUM_SCRIPTED_SCENE_HPP
#define CADMIUM_SCRIPTED_SCENE_HPP
#include <cadmium/core/scene.hpp>
#include <cadmium/scripting/entity_registry.hpp>
#include <string>
#include <sol/sol.hpp>

namespace Cadmium
{

// A scene entirely driven by a Lua script.
// The script defines OnEnter, OnExit, OnUpdate, OnRender etc as globals.
// No C++ subclassing required.
class ScriptedScene : public Scene
{
public:
    explicit ScriptedScene(std::string scriptPath)
        : Scene("ScriptedScene")
        , m_ScriptPath(std::move(scriptPath))
    {}

    void OnEnter() override;
    void OnExit()  override;
    void OnDestroy() override;

private:
    std::string m_ScriptPath{};
    sol::environment m_Env{};
    EntityRegistry m_EntityRegistry;
};

} // namespace Cadmium
#endif // CADMIUM_SCRIPTED_SCENE_HPP
