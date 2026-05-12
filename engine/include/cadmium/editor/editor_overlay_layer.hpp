#ifndef CADMIUM_EDITOR_OVERLAY_LAYER_HPP
#define CADMIUM_EDITOR_OVERLAY_LAYER_HPP

#include <cadmium/core/layer.hpp>
#include <cadmium/editor/editor_state.hpp>
#include <cadmium/editor/toolbar_panel.hpp>
#include <cadmium/editor/console_panel.hpp>
#include <cadmium/editor/script_editor_panel.hpp>
#include <cadmium/assets/asset_manager.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Cadmium
{
    class ScriptedScene;
}

namespace Cadmium::Editor
{

class EditorOverlayLayer : public Layer
{
public:
    EditorOverlayLayer(AssetManager& assets, ScriptedScene& scene)
        : Layer("EditorOverlayLayer")
        , m_ScriptPanel(assets)
        , m_Scene(scene)
    {}

    //  Layer callbacks

    void OnAttach() override
    {
        m_Console.Attach();

        // Start in Edit mode - script is paused until user hits Play
        m_Scene.SetScriptPaused(true);
    }

    void OnDetach() override
    {
        m_Console.Detach();
    }

    void OnUpdate(float) override
    {
        // Only poll run request in Edit mode
        if (m_State != EditorState::Edit) return;

        std::string source;
        if (m_ScriptPanel.ConsumeRunRequest(source))
            TransitionToPlay(source);
    }

    void OnImGuiRender() override
    {
#ifdef CADMIUM_IMGUI
        bool stateChanged = m_Toolbar.Render(m_State);

        if (stateChanged)
            OnStateChanged();

        if (m_State == EditorState::Edit)
            RenderEditPanels();
#endif
    }

    //  State access

    EditorState        GetState() const  { return m_State;       }
    ScriptEditorPanel& GetScriptPanel()  { return m_ScriptPanel; }

private:

    void OnStateChanged()
    {
        switch (m_State)
        {
            case EditorState::Edit:
                // Stop pressed - pause script, keep last frame visible
                m_Scene.SetScriptPaused(true);
                break;

            case EditorState::Play:
                // Play pressed directly from toolbar (no source change)
                // Just unpause - script state is whatever it was
                m_Scene.SetScriptPaused(false);
                break;
        }
    }

    void TransitionToPlay(const std::string& source)
    {
        bool ok = m_Scene.Reload(source);

        if (!ok)
        {
            m_ScriptPanel.SetError("Script failed to execute. Check console.");
            // Stay in Edit mode on failure
            m_State = EditorState::Edit;
            return;
        }

        m_ScriptPanel.ClearError();
        m_State = EditorState::Play;
        m_Scene.SetScriptPaused(false);
    }

#ifdef CADMIUM_IMGUI

    void RenderEditPanels()
    {
        ImGuiIO& io = ImGui::GetIO();

        float toolbarH = ToolbarPanel::k_ToolbarHeight;

        // Script editor - right side, full height below toolbar
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.55f, toolbarH),
            ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(
            ImVec2(io.DisplaySize.x * 0.45f,
                   io.DisplaySize.y - toolbarH),
            ImGuiCond_FirstUseEver);

        m_ScriptPanel.Render("Script Editor");

        // Console - bottom left, below game view
        ImGui::SetNextWindowPos(
            ImVec2(0.f, io.DisplaySize.y * 0.65f),
            ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(
            ImVec2(io.DisplaySize.x * 0.55f,
                   io.DisplaySize.y * 0.35f),
            ImGuiCond_FirstUseEver);

        m_Console.Render("Console");
    }

#endif

    //  Data

    EditorState       m_State{EditorState::Edit};
    ToolbarPanel      m_Toolbar;
    ConsolePanel      m_Console;
    ScriptEditorPanel m_ScriptPanel;
    ScriptedScene&    m_Scene;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_OVERLAY_LAYER_HPP
