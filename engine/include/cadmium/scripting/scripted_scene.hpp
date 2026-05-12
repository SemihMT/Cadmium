#ifndef CADMIUM_SCRIPTED_SCENE_HPP
#define CADMIUM_SCRIPTED_SCENE_HPP

#include <cadmium/core/scene.hpp>
#include <cadmium/scripting/entity_registry.hpp>
#include <string>
#include <sol/sol.hpp>

namespace Cadmium
{

class ScriptUpdateLayer; // forward declare

class ScriptedScene : public Scene
{
public:
    static std::unique_ptr<ScriptedScene> FromFile(std::string path)
    {
        return std::unique_ptr<ScriptedScene>(
            new ScriptedScene(std::move(path), {}, Mode::File));
    }

    static std::unique_ptr<ScriptedScene> FromSource(std::string name,
                                                      std::string source)
    {
        return std::unique_ptr<ScriptedScene>(
            new ScriptedScene(std::move(name), std::move(source),
                              Mode::Source));
    }

    void OnEnter()   override;
    void OnExit()    override;
    void OnDestroy() override;

    // Called by EditorOverlayLayer on state transitions.
    // Pauses/resumes script update ticks without touching rendering.
    void SetScriptPaused(bool paused);

    // Reload from new source text.
    // Resets entity state and re-executes from scratch.
    // Returns false if execution fails.
    bool Reload(const std::string& source);

    void EnableEditor(AssetManager& assets)
    {
        m_EditorAssets = &assets;
    }

private:
    enum class Mode { File, Source };

    ScriptedScene(std::string nameOrPath, std::string source, Mode mode)
        : Scene("ScriptedScene")
        , m_NameOrPath{std::move(nameOrPath)}
        , m_Source{std::move(source)}
        , m_Mode{mode}
    {}

    bool Execute();

    std::string         m_NameOrPath{};
    std::string         m_Source{};
    Mode                m_Mode{Mode::File};
    sol::environment    m_Env{};
    EntityRegistry      m_EntityRegistry;
    ScriptUpdateLayer*  m_UpdateLayer{nullptr}; // non-owning, set in OnEnter
    AssetManager*       m_EditorAssets{nullptr};
};

} // namespace Cadmium

#endif // CADMIUM_SCRIPTED_SCENE_HPP
