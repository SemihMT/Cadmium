#ifndef CADMIUM_EDITOR_OVERLAY_LAYER_HPP
#define CADMIUM_EDITOR_OVERLAY_LAYER_HPP

#include <cadmium/core/layer.hpp>
#include <cadmium/assets/asset_manager.hpp>
#include <cadmium/assets/asset_types.hpp>
#include <cadmium/scripting/script_controller.hpp>
#include <cadmium/editor/script_editor_panel.hpp>
#include <cadmium/editor/asset_panel.hpp>
#include <cadmium/editor/console_panel.hpp>
#include <cadmium/editor/toolbar_panel.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace Cadmium::Editor
{

class EditorOverlayLayer : public Layer
{
public:
    EditorOverlayLayer(AssetManager& assets, IScriptController& controller)
        : Layer("EditorOverlayLayer")
        , m_Assets(assets)
        , m_Controller(controller)
        , m_ScriptPanel(assets)
        , m_AssetPanel(assets)
    {
        m_AssetPanel.SetOnSelect(
            [this](const std::string& relativePath, AssetType type)
            {
                if (type != AssetType::Script) return;
                m_ScriptPanel.OpenScript(m_Assets.ResolvePath(relativePath));
            });
    }

    void OnAttach() override { m_Console.Attach(); }
    void OnDetach() override { m_Console.Detach(); }

    void OnImGuiRender() override
    {
#ifdef CADMIUM_IMGUI
        EditorState stateBefore = m_State;
        m_Toolbar.Render(m_State);
        if (m_State != stateBefore)
            m_Controller.Pause(m_State == EditorState::Edit);

        std::string source;
        if (m_ScriptPanel.ConsumeRunRequest(source))
        {
            bool ok = m_Controller.Reload(source);
            if (!ok)
                m_ScriptPanel.SetError("Script error — check console");
            else
                m_ScriptPanel.ClearError();
        }

        SetupDockspace();

        m_AssetPanel.Render("Assets");
        m_ScriptPanel.Render("Script Editor");
        m_Console.Render("Console");
#endif
    }

    ScriptEditorPanel& GetScriptPanel() { return m_ScriptPanel; }

private:
#ifdef CADMIUM_IMGUI
    void SetupDockspace()
    {
        ImGuiIO& io = ImGui::GetIO();

        float toolbarH = ToolbarPanel::k_ToolbarHeight;
        ImGui::SetNextWindowPos({ 0.f, toolbarH });
        ImGui::SetNextWindowSize({ io.DisplaySize.x,
                                   io.DisplaySize.y - toolbarH });
        ImGui::SetNextWindowBgAlpha(0.f);

        ImGuiWindowFlags hostFlags =
            ImGuiWindowFlags_NoTitleBar          |
            ImGuiWindowFlags_NoCollapse          |
            ImGuiWindowFlags_NoResize            |
            ImGuiWindowFlags_NoMove              |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus          |
            ImGuiWindowFlags_NoDocking           |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::Begin("##DockspaceHost", nullptr, hostFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockId = ImGui::GetID("##MainDockspace");
        ImGui::DockSpace(dockId, { 0.f, 0.f }, ImGuiDockNodeFlags_PassthruCentralNode);

        if (!m_DockLayoutBuilt)
        {
            BuildDefaultLayout(dockId);
            m_DockLayoutBuilt = true;
        }

        ImGui::End();
    }

    void BuildDefaultLayout(ImGuiID dockId)
    {
        ImGui::DockBuilderRemoveNode(dockId);
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(dockId, ImGui::GetMainViewport()->Size);

        //  Split: left sidebar | remainder
        ImGuiID leftId, centerId;
        ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Left, 0.22f,
                                    &leftId, &centerId);

        //  Split remainder: editor (top) | console (bottom)
        ImGuiID editorId, consoleId;
        ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.30f,
                                    &consoleId, &editorId);

        ImGui::DockBuilderDockWindow("Assets",        leftId);
        ImGui::DockBuilderDockWindow("Script Editor", editorId);
        ImGui::DockBuilderDockWindow("Console",       consoleId);

        ImGui::DockBuilderFinish(dockId);
    }
#endif

    AssetManager&      m_Assets;
    IScriptController& m_Controller;
    EditorState        m_State{EditorState::Edit};
    bool               m_DockLayoutBuilt{false};

    ScriptEditorPanel  m_ScriptPanel;
    AssetPanel         m_AssetPanel;
    ConsolePanel       m_Console;
    ToolbarPanel       m_Toolbar;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_OVERLAY_LAYER_HPP
