#ifndef CADMIUM_EDITOR_OVERLAY_LAYER_HPP
#define CADMIUM_EDITOR_OVERLAY_LAYER_HPP

#include "cadmium/render/renderer.hpp"
#include <cadmium/editor/register_builtin_components.hpp>
#include <cadmium/assets/asset_manager.hpp>
#include <cadmium/assets/asset_types.hpp>
#include <cadmium/core/layer.hpp>
#include <cadmium/core/logger.hpp>
#include <cadmium/core/scene.hpp>
#include <cadmium/editor/asset_panel.hpp>
#include <cadmium/editor/component_commands.hpp>
#include <cadmium/editor/console_panel.hpp>
#include <cadmium/editor/editor_dirty_state.hpp>
#include <cadmium/editor/editor_selection.hpp>
#include <cadmium/editor/hierarchy_panel.hpp>
#include <cadmium/editor/inspector_panel.hpp>
#include <cadmium/editor/render_viewport.hpp>
#include <cadmium/editor/script_commands.hpp>
#include <cadmium/editor/script_editor_panel.hpp>
#include <cadmium/editor/script_hot_reload.hpp>
#include <cadmium/editor/toolbar_panel.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <cadmium/editor/viewport_panel.hpp>
#include <cadmium/editor/world_snapshot.hpp>
#include <cadmium/scripting/script_host.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace Cadmium::Editor
{

    class EditorOverlayLayer : public Cadmium::Layer
    {
      public:
        EditorOverlayLayer(AssetManager& assets, IRenderer& renderer, IEngineContext* context)
            : Layer("EditorOverlayLayer"), m_Assets(assets), m_AssetPanel(assets, renderer),
              m_ScriptEditor(assets), m_Context(context),
              m_ViewportPanel([this](int w, int h) { m_Context->ResizeViewport(w, h); })
        {
            m_AssetPanel.SetOnSelect(
                [this](const std::string& relativePath, AssetType type)
                {
                    // Route through the shared selection context (not just
                    // the script editor) so the Asset panel keeps this file
                    // highlighted even though the open came from a
                    // double-click, not the panel's own selection logic.
                    m_Selection.SelectAsset(relativePath);
                    if (type == AssetType::Script)
                        m_ScriptEditor.OpenScript(relativePath);
                });

            m_HierarchyPanel.SetOnAssetDropped(
                [this](Entity e, const std::string& path, AssetType type)
                {
                    m_Selection.SelectEntity(e);
                    m_Selection.SelectAsset(path);

                    if (type == AssetType::Script)
                        if (Scene* scene = m_Context->GetActiveScene())
                            AttachScript(scene->GetWorld(), e, path);
                });

            // Any command that lands on the undo stack (component edit,
            // entity create/rename/reparent) marks the scene dirty - one
            // hook instead of every call site remembering to do it.
            // Delete is the one exception (see HierarchyPanel::
            // DeleteSelection) and marks dirty directly since it doesn't
            // go through the stack.
            UndoStack::Get().SetOnChanged([this] { m_DirtyState.SetDirty(); });

            m_Inspector.SetOnScriptDropped(
                [this](World& world, Entity e, const std::string& scriptPath)
                {
                    AttachScript(world, e, scriptPath);
                });
        }

        void OnAttach() override
        {
            Detail::RegisterBuiltinComponents();
            m_Console.Attach();
            m_Context->EnableViewport(1280, 720);
            m_Context->SetSimulationPaused(true);
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

            // Global Ctrl+Z / Ctrl+Y. Skipped while a text field has focus
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantTextInput)
            {
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
                    UndoStack::Get().Undo();
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
                    UndoStack::Get().Redo();
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
                {
                    Log::Warn("EditorOverlayLayer", "Save Scene: serialization not yet implemented "
                                                    "- clearing the unsaved indicator only.");
                    m_DirtyState.ClearDirty();
                }
            }

            float menuBarH = RenderMainMenuBar();

            bool stateChanged = m_Toolbar.Render(m_State, menuBarH);
            if (stateChanged)
                HandlePlayStateChange(stateBefore);

            if (Scene* scene = m_Context->GetActiveScene())
                m_ScriptHotReload.Update(scene->GetWorld(), io.DeltaTime);

            SetupDockspace(menuBarH);

            m_AssetPanel.Render(m_Selection, "Assets", &m_ShowAssets);
            m_Console.Render("Console", &m_ShowConsole);
            {
                Scene* scene = m_Context->GetActiveScene();
                World* world = scene ? &scene->GetWorld() : nullptr;
                Entity gizmoTarget = (m_Selection.GetMultiSelection().size() == 1)
                    ? m_Selection.GetEntity()
                    : k_NullEntity;
                m_ViewportPanel.Render(m_Context->GetViewport(), world, gizmoTarget, &m_Context->GetRenderer(),
                                       "Viewport", &m_ShowViewport);
            }
            m_ScriptEditor.Render("Script Editor", &m_ShowScriptEditor);

            if (Scene* scene = m_Context->GetActiveScene())
            {
                m_HierarchyPanel.Render(scene->GetWorld(), m_Selection, m_DirtyState,
                                        scene->GetName(), "Hierarchy", &m_ShowHierarchy);
                m_Inspector.Render(scene->GetWorld(), m_Selection, "Inspector", &m_ShowInspector);
            }
#endif
        }

      private:
        // Captures/restores the scene around Play so gameplay mutations
        // never leak into the authored scene
        void HandlePlayStateChange(EditorState stateBefore)
        {
            Scene* scene = m_Context->GetActiveScene();
            if (!scene)
                return;

            World& world = scene->GetWorld();

            if (m_State == EditorState::Play && stateBefore == EditorState::Edit)
            {
                m_WorldSnapshot.Capture(world);
                m_Context->SetSimulationPaused(false);
                Log::Info("EditorOverlayLayer", "Entering Play - scene snapshot captured.");
            }
            else if (m_State == EditorState::Edit && stateBefore == EditorState::Play)
            {
                m_Context->SetSimulationPaused(true);
                m_WorldSnapshot.Restore(world);
                // Every Entity handle from before Stop (selection, hot-
                // reload tracking) is now potentially dangling or - worse -
                // silently pointing at a different, newly-recreated entity.
                m_Selection.ClearEntity();
                m_ScriptHotReload.Clear();
                Log::Info("EditorOverlayLayer", "Stopped - scene restored to its pre-Play state.");
            }
        }

        // Shared by both drop paths (Inspector's whole-window target and
        // Hierarchy's per-row target) so there's one place that knows how
        // to turn a script asset path into a live ScriptInstance.
        void AttachScript(World& world, Entity e, const std::string& scriptPath)
        {
            m_Selection.SelectEntity(e);
            m_Selection.SelectAsset(scriptPath);
            m_ScriptEditor.OpenScript(scriptPath);

            if (!world.HasComponent<Script>(e))
                UndoStack::Get().ExecuteAndPush(std::make_unique<AddComponentCommand<Script>>(world, e, "Script"));

            std::string resolvedPath = m_Assets.ResolvePath(scriptPath);
            ScriptHost::LoadedScript loaded = world.GetScriptHost().LoadScript(resolvedPath);
            if (!loaded.valid)
            {
                Log::Error("EditorOverlayLayer", "Failed to attach script '{}' - see console for the load error.", scriptPath);
                return;
            }

            ScriptInstance instance;
            instance.env = loaded.env;
            instance.env["self"] = EntityHandle{&world, e};
            instance.name = loaded.name;
            instance.onStart = loaded.onStart;
            instance.onUpdate = loaded.onUpdate;
            instance.onRender = loaded.onRender;
            instance.onDestroy = loaded.onDestroy;
            instance.fieldMetadata = std::move(loaded.fieldMetadata);
            instance.fieldOrder = std::move(loaded.fieldOrder);

            UndoStack::Get().ExecuteAndPush(
                std::make_unique<AddScriptInstanceCommand>(world, e, std::move(instance)));

            size_t newIndex = world.GetComponent<Script>(e).instances.size() - 1;
            m_ScriptHotReload.TrackInstance(resolvedPath, e, newIndex);
        }

#ifdef CADMIUM_IMGUI
        // Renders File / Edit / View / Window as a real ImGui main menu bar.
        // Returns its height, so the toolbar and dockspace below it can
        // offset themselves instead of overlapping.
        float RenderMainMenuBar()
        {
            float height = 0.f;

            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    // TODO: Scene serialization doesn't exist yet
                    if (ImGui::MenuItem("New Scene"))
                        Log::Warn("EditorOverlayLayer", "New Scene: not yet implemented.");
                    if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                    {
                        Log::Warn("EditorOverlayLayer", "Save Scene: serialization not yet implemented "
                                                        "- clearing the unsaved indicator only.");
                        m_DirtyState.ClearDirty();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Reload All Scripts"))
                        if (Scene* scene = m_Context->GetActiveScene())
                            m_ScriptHotReload.ReloadAll(scene->GetWorld());
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit"))
                {
                    bool canUndo = UndoStack::Get().CanUndo();
                    bool canRedo = UndoStack::Get().CanRedo();

                    std::string undoLabel = "Undo";
                    if (canUndo)
                        if (auto desc = UndoStack::Get().PeekUndoLabel(); !desc.empty())
                            undoLabel += " " + desc;

                    std::string redoLabel = "Redo";
                    if (canRedo)
                        if (auto desc = UndoStack::Get().PeekRedoLabel(); !desc.empty())
                            redoLabel += " " + desc;

                    if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUndo))
                        UndoStack::Get().Undo();
                    if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, canRedo))
                        UndoStack::Get().Redo();
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    if (ImGui::MenuItem("Reset Layout"))
                        m_DockLayoutBuilt = false;
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Window"))
                {
                    ImGui::MenuItem("Assets", nullptr, &m_ShowAssets);
                    ImGui::MenuItem("Hierarchy", nullptr, &m_ShowHierarchy);
                    ImGui::MenuItem("Inspector", nullptr, &m_ShowInspector);
                    ImGui::MenuItem("Viewport", nullptr, &m_ShowViewport);
                    ImGui::MenuItem("Script Editor", nullptr, &m_ShowScriptEditor);
                    ImGui::MenuItem("Console", nullptr, &m_ShowConsole);
                    ImGui::EndMenu();
                }

                height = ImGui::GetWindowSize().y;
                ImGui::EndMainMenuBar();
            }

            return height;
        }

        void SetupDockspace(float menuBarH)
        {
            ImGuiIO& io = ImGui::GetIO();

            float toolbarH = ToolbarPanel::k_ToolbarHeight;
            float topOffset = menuBarH + toolbarH;

            ImGui::SetNextWindowPos({0.f, topOffset});
            ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y - topOffset});
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
            ImGui::DockBuilderDockWindow("Script Editor", gameId);
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
        ScriptEditorPanel m_ScriptEditor;
        ConsolePanel m_Console;
        ToolbarPanel m_Toolbar;
        ViewportPanel m_ViewportPanel;
        HierarchyPanel m_HierarchyPanel;
        InspectorPanel m_Inspector;
        EditorSelectionContext m_Selection;
        EditorDirtyState m_DirtyState;
        WorldSnapshotService m_WorldSnapshot;
        ScriptHotReloadService m_ScriptHotReload;

        // Window-menu visibility toggles. All default open
        bool m_ShowAssets{true};
        bool m_ShowHierarchy{true};
        bool m_ShowInspector{true};
        bool m_ShowViewport{true};
        bool m_ShowScriptEditor{true};
        bool m_ShowConsole{true};
    };

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_OVERLAY_LAYER_HPP
