#ifndef CADMIUM_SCRIPTED_SCENE_HPP
#define CADMIUM_SCRIPTED_SCENE_HPP

#include <cadmium/core/scene.hpp>
#include <cadmium/scripting/script_host.hpp>
#include <cadmium/scripting/entity_registry.hpp>
#include <string>
#include <sol/sol.hpp>

namespace Cadmium
{

class ScriptUpdateLayer;

class ScriptedScene : public Scene
{
public:
    static std::unique_ptr<ScriptedScene> FromFile(const std::string& path);

    static std::unique_ptr<ScriptedScene> FromSource(const std::string &name,
                                                     const std::string &source);

    void OnEnter()   override;
    void OnExit()    override;
    void OnDestroy() override;

    IScriptController& GetController() { return *m_Host; }

    void EnableEditor(AssetManager& assets) { m_EditorAssets = &assets;}

private:

    ScriptedScene(const std::string& nameOrPath, std::string source)
        : Scene("ScriptedScene")
        , m_NameOrPath{std::move(nameOrPath)}
        , m_Source{std::move(source)}
    {}

    std::string         m_NameOrPath{};
    std::string         m_Source{};
    std::unique_ptr<ScriptHost> m_Host;
    AssetManager*       m_EditorAssets{nullptr};
};

} // namespace Cadmium

#endif // CADMIUM_SCRIPTED_SCENE_HPP
