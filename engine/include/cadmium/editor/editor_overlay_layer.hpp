#ifndef CADMIUM_EDITOR_OVERLAY_LAYER_HPP
#define CADMIUM_EDITOR_OVERLAY_LAYER_HPP

#include <cadmium/editor/register_builtin_components.hpp>
#include <cadmium/assets/asset_manager.hpp>
#include <cadmium/assets/asset_types.hpp>
#include <cadmium/core/layer.hpp>
#include <cadmium/core/scene.hpp>
#include <cadmium/editor/asset_panel.hpp>
#include <cadmium/editor/console_panel.hpp>
#include <cadmium/editor/hierarchy_panel.hpp>
#include <cadmium/editor/inspector_panel.hpp>
#include <cadmium/editor/render_viewport.hpp>
#include <cadmium/editor/toolbar_panel.hpp>
#include <cadmium/editor/viewport_panel.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace Cadmium::Editor
{

    class EditorOverlayLayer : public Layer
    {
      public:
        EditorOverlayLayer(AssetManager& assets, IEngineContext* context)
            : Layer("EditorOverlayLayer"), m_Assets(assets), m_AssetPanel(assets),
              m_Context(context),
              m_ViewportPanel([this](int w, int h) { m_Context->ResizeViewport(w, h); })
        {
            m_AssetPanel.SetOnSelect([this](const std::string& relativePath, AssetType type) {});
        }

        void OnAttach() override
        {
            Detail::RegisterBuiltinComponents();
            m_Console.Attach();
            m_Context->EnableViewport(1280, 720);
        }
        void OnDetach() override
        {
            m_Console.Detach();
            m_Context->DisableViewport();
        }

        void OnImGuiRender() override
        {
#ifdef CADMIUM_IMGUI
            EditorState stateBefore = m_State;
            m_Toolbar.Render(m_State);
            // if (m_State != stateBefore)
            //     m_Controller.Pause(m_State == EditorState::Edit);

            // std::string source;
            // if (m_ScriptPanel.ConsumeRunRequest(source))
            // {
            //     bool ok = m_Controller.Reload(source);
            //     if (!ok)
            //         m_ScriptPanel.SetError("Script error — check console");
            //     else
            //         m_ScriptPanel.ClearError();
            // }

            SetupDockspace();

            m_AssetPanel.Render("Assets");
            m_Console.Render("Console");
            m_ViewportPanel.Render(m_Context->GetViewport(), "Viewport");

            if (Scene* scene = m_Context->GetActiveScene())
            {
                m_HierarchyPanel.Render(scene->GetWorld(), "Hierarchy");
                m_Inspector.Render(scene->GetWorld(), m_HierarchyPanel.GetSelected(), "Inspector");
            }
#endif
        }

      private:
#ifdef CADMIUM_IMGUI
        void SetupDockspace()
        {
            ImGuiIO& io = ImGui::GetIO();

            float toolbarH = ToolbarPanel::k_ToolbarHeight;
            ImGui::SetNextWindowPos({0.f, toolbarH});
            ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y - toolbarH});
            ImGui::SetNextWindowBgAlpha(0.f);

            ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            ImGui::Begin("##DockspaceHost", nullptr, hostFlags);
            ImGui::PopStyleVar(3);

            ImGuiID dockId = ImGui::GetID("##MainDockspace");
            ImGui::DockSpace(dockId, {0.f, 0.f}, ImGuiDockNodeFlags_PassthruCentralNode);

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

            // Split: left | center
            ImGuiID leftId, rightId;
            ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Left, 0.20f, &leftId, &rightId);

            // Split left column: Assets (top) | Hierarchy (bottom)
            ImGuiID leftTopId, leftBottomId;
            ImGui::DockBuilderSplitNode(leftId, ImGuiDir_Down, 0.5f, &leftBottomId, &leftTopId);

            // Split center: viewport area (top) | console (bottom)
            ImGuiID viewportId, bottomId;
            ImGui::DockBuilderSplitNode(rightId, ImGuiDir_Down, 0.25f, &bottomId, &viewportId);

            // Split viewport area: game view (left) | inspector (right)
            ImGuiID gameId, inspectorId;
            ImGui::DockBuilderSplitNode(viewportId, ImGuiDir_Right, 0.30f, &inspectorId, &gameId);

            ImGui::DockBuilderDockWindow("Assets", leftTopId);
            ImGui::DockBuilderDockWindow("Hierarchy", leftBottomId);
            ImGui::DockBuilderDockWindow("Viewport", gameId);
            ImGui::DockBuilderDockWindow("Inspector", inspectorId);
            ImGui::DockBuilderDockWindow("Console", bottomId);

            ImGui::DockBuilderFinish(dockId);
        }
#endif

        AssetManager& m_Assets;
        IEngineContext* m_Context{nullptr};
        EditorState m_State{EditorState::Edit};
        bool m_DockLayoutBuilt{false};

        AssetPanel m_AssetPanel;
        ConsolePanel m_Console;
        ToolbarPanel m_Toolbar;
        ViewportPanel m_ViewportPanel;
        HierarchyPanel m_HierarchyPanel;
        InspectorPanel m_Inspector;
    };

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_OVERLAY_LAYER_HPP
