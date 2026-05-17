#include <cadmium/scripting/scripted_scene.hpp>
#include <cadmium/scripting/script_update_layer.hpp>
#include <cadmium/scripting/script_render_layer.hpp>
#include <cadmium/editor/editor_overlay_layer.hpp>

namespace Cadmium {

std::unique_ptr<ScriptedScene> ScriptedScene::FromFile(const std::string& path)
{
    // Read source from disk here, once. Scene no longer needs to know it's a file.
    std::ifstream f(path);
    std::string source((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    return std::unique_ptr<ScriptedScene>(
        new ScriptedScene(std::move(path), std::move(source)));
}

std::unique_ptr<ScriptedScene> ScriptedScene::FromSource(const std::string& name,
                                                          const std::string& source)
{
    return std::unique_ptr<ScriptedScene>(
        new ScriptedScene(std::move(name), std::move(source)));
}

void ScriptedScene::OnEnter()
{
    m_Host = std::make_unique<ScriptHost>();
    m_Host->BindAPIs(GetWorld(), GetAssets(), GetDrawQueue(), GetInput(), GetSceneState());

    PushLayer(std::make_unique<ScriptUpdateLayer>(*m_Host));
    PushLayer(std::make_unique<ScriptRenderLayer>(
        GetDrawQueue(), GetAssets(), GetFont()));

    if (m_EditorAssets) {
        auto overlay = std::make_unique<Editor::EditorOverlayLayer>(
            *m_EditorAssets, GetController());
        PushOverlay(std::move(overlay));
    }

    m_Host->Load(m_Source, m_NameOrPath);
}

void ScriptedScene::OnExit()
{
    sol::protected_function f = m_Host->GetEnv()["OnExit"];
    if (f.valid()) f();
}

void ScriptedScene::OnDestroy()
{
    if (m_Host) {
        sol::protected_function f = m_Host->GetEnv()["OnDestroy"];
        if (f.valid()) f();
        m_Host.reset();
    }
}

} // namespace Cadmium
