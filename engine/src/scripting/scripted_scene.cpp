#include "cadmium/scripting/scripted_scene.hpp"
#include "cadmium/scripting/script_render_layer.hpp"
#include "cadmium/scripting/script_update_layer.hpp"
#include "cadmium/scripting/lua_bindings_entity.hpp"
#include "cadmium/scripting/lua_bindings_components.hpp"
#include "cadmium/core/logger.hpp"
#include "cadmium/editor/editor_overlay_layer.hpp"

namespace Cadmium
{

void ScriptedScene::OnEnter()
{
    auto& lua    = GetLua();
    auto& ecsReg = GetWorld().GetRegistry();

    m_Env = sol::environment(lua, sol::create, lua.globals());

    Lua::BindEntity(lua, m_EntityRegistry, ecsReg);
    Lua::BindComponents(lua, GetWorld());

    auto updateLayer = std::make_unique<ScriptUpdateLayer>(
        m_Env, m_EntityRegistry);
    m_UpdateLayer = updateLayer.get(); // cache non-owning pointer

    PushLayer(std::move(updateLayer));
    PushLayer(std::make_unique<ScriptRenderLayer>(
        GetDrawQueue(), GetAssets(), GetFont()));

    if (m_EditorAssets)
    {
      auto overlay = std::make_unique<Editor::EditorOverlayLayer>(
          *m_EditorAssets, *this);

      // If we have a file path, load it into the editor panel
      if (m_Mode == Mode::File)
      {
        // TODO: Fix all asset loading code to be relative to ./assets/ instead of hacking like this
        std::string relativePath = m_NameOrPath;
        const std::string prefix = "assets/";
        if (relativePath.starts_with(prefix))
          relativePath = relativePath.substr(prefix.size());

        ScriptHandle handle = m_EditorAssets->LoadScript(relativePath);
        if (handle != k_InvalidHandle)
          overlay->GetScriptPanel().OpenScript(handle);
      }

      PushOverlay(std::move(overlay));
    }

    if (!Execute())
        Log::Error("Scripting", "Failed to load '{}'", m_NameOrPath);
}

void ScriptedScene::OnExit()
{
    sol::protected_function f = m_Env["OnExit"];
    if (f.valid()) f();
}

void ScriptedScene::OnDestroy()
{
    sol::protected_function f = m_Env["OnDestroy"];
    if (f.valid()) f();

    m_EntityRegistry.Clear(GetWorld());
    m_Env         = sol::environment{};
    m_UpdateLayer = nullptr;
}

void ScriptedScene::SetScriptPaused(bool paused)
{
    if (m_UpdateLayer)
        m_UpdateLayer->SetPaused(paused);
}

bool ScriptedScene::Reload(const std::string& source)
{
    m_Source = source;
    m_Mode   = Mode::Source;

    // Teardown existing script state
    sol::protected_function onDestroy = m_Env["OnDestroy"];
    if (onDestroy.valid()) onDestroy();

    m_EntityRegistry.Clear(GetWorld());

    // Swap game layers - editor overlay stays untouched
    PopLayerImmediate("ScriptUpdateLayer");
    PopLayerImmediate("ScriptRenderLayer");

    // Fresh environment
    auto& lua    = GetLua();
    auto& ecsReg = GetWorld().GetRegistry();
    m_Env        = sol::environment(lua, sol::create, lua.globals());

    Lua::BindEntity(lua, m_EntityRegistry, ecsReg);
    Lua::BindComponents(lua, GetWorld());

    auto updateLayer = std::make_unique<ScriptUpdateLayer>(
        m_Env, m_EntityRegistry);
    m_UpdateLayer = updateLayer.get();

    PushLayer(std::move(updateLayer));
    PushLayer(std::make_unique<ScriptRenderLayer>(
        GetDrawQueue(), GetAssets(), GetFont()));

    if (!Execute())
    {
        Log::Error("Scripting", "Reload failed for '{}'", m_NameOrPath);
        m_UpdateLayer = nullptr;
        return false;
    }

    return true;
}

bool ScriptedScene::Execute()
{
    auto& lua = GetLua();

    sol::protected_function_result result;

    if (m_Mode == Mode::File)
    {
        result = lua.safe_script_file(
            m_NameOrPath,
            m_Env,
            sol::script_pass_on_error);
    }
    else
    {
        result = lua.safe_script(
            m_Source,
            m_Env,
            sol::script_pass_on_error,
            "@" + m_NameOrPath);
    }

    if (!result.valid())
    {
        sol::error err = result;
        Log::Error("Scripting", "{}", err.what());
        return false;
    }

    sol::protected_function onEnter = m_Env["OnEnter"];
    if (onEnter.valid())
    {
        auto r = onEnter();
        if (!r.valid())
        {
            sol::error err = r;
            Log::Error("Scripting", "OnEnter error: {}", err.what());
            return false;
        }
    }

    return true;
}

} // namespace Cadmium
