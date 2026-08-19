#ifndef CADMIUM_EDITOR_HIERARCHY_PANEL
#define CADMIUM_EDITOR_HIERARCHY_PANEL
#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/asset_drag_payload.hpp>
#include <cadmium/editor/editor_dirty_state.hpp>
#include <cadmium/editor/editor_selection.hpp>
#include <cadmium/editor/entity_drag_payload.hpp>
#include <cadmium/editor/hierarchy_commands.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <algorithm>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Cadmium::Editor
{
    class HierarchyPanel
    {
      public:
        // Called when an asset is dropped on an entity row. Owner decides
        // what to do with it (e.g. attach script).
        using AssetDropCallback = std::function<void(Entity, const std::string& path, AssetType)>;

        void SetOnAssetDropped(AssetDropCallback cb) { m_OnAssetDropped = std::move(cb); }

        // world: the active scene's world.
        // selection: shared across panels. dirtyState: set automatically via
        // UndoStack hook, except Delete (see DeleteSelection). open: optional
        // visibility toggle (nullptr = always render).
        void Render(World& world, EditorSelectionContext& selection, EditorDirtyState& dirtyState,
                    const std::string& sceneName, const char* title = "Hierarchy", bool* open = nullptr)
        {
#ifdef CADMIUM_IMGUI
            if (open && !*open)
                return;

            if (!ImGui::Begin(title, open))
            {
                ImGui::End();
                return;
            }

            RenderToolbar(world, selection, sceneName, dirtyState);
            ImGui::Separator();

            // Rebuilt each frame for correct shift-click range selection.
            m_FlatOrder = BuildFlatOrder(world);

            for (Entity e : world.AllEntities())
                if (!world.HasComponent<Parent>(e))
                    DrawEntityNode(world, e, selection, dirtyState);

            RenderEmptyAreaDropZone(world, selection);

            HandleShortcuts(world, selection, dirtyState);

            ImGui::End();
#endif
        }

        void DrawEntityNode(World& world, Entity e, EditorSelectionContext& selection, EditorDirtyState& dirtyState)
        {
#ifdef CADMIUM_IMGUI
            bool isRenaming = (m_RenamingEntity == e);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (selection.IsSelected(e))
                flags |= ImGuiTreeNodeFlags_Selected;
            if (isRenaming)
                flags |= ImGuiTreeNodeFlags_AllowOverlap;

            std::string label = DisplayName(world, e);
            std::string nodeId = "##node_" + std::to_string(EntityIndex(e));

            bool open = ImGui::TreeNodeEx(isRenaming ? nodeId.c_str() : label.c_str(), flags);

            if (isRenaming)
            {
                ImGui::SameLine();
                if (m_JustStartedRename)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_JustStartedRename = false;
                }
                ImGui::SetNextItemWidth(160.f);
                bool entered = ImGui::InputText("##rename", m_RenameBuf, sizeof(m_RenameBuf),
                                                ImGuiInputTextFlags_EnterReturnsTrue |
                                                ImGuiInputTextFlags_AutoSelectAll);
                if (entered || ImGui::IsItemDeactivatedAfterEdit())
                    CommitRename(world, e);
                else if (ImGui::IsItemDeactivated())
                    m_RenamingEntity = k_NullEntity; // cancelled (e.g. Escape)
            }
            else
            {
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    StartRename(world, e);
                }
                else if (ImGui::IsItemClicked())
                {
                    ImGuiIO& io = ImGui::GetIO();
                    if (io.KeyShift && selection.GetEntity() != k_NullEntity)
                        selection.SetMultiSelection(ComputeRange(m_FlatOrder, selection.GetEntity(), e));
                    else if (io.KeyCtrl)
                        selection.ToggleEntity(e);
                    else
                        selection.SelectEntity(e);
                }

                // Drag source: drags multi-selection if this row is part of one.
                if (ImGui::BeginDragDropSource())
                {
                    std::vector<Entity> dragged = (selection.IsSelected(e) && selection.GetMultiSelection().size() > 1)
                        ? selection.GetMultiSelection()
                        : std::vector<Entity>{e};
                    EntityDragPayload payload = EntityDragPayload::Make(dragged);
                    ImGui::SetDragDropPayload(k_EntityDragDropId, &payload, sizeof(payload));
                    ImGui::Text("%s", label.c_str());
                    if (dragged.size() > 1)
                        ImGui::Text("+%zu more", dragged.size() - 1);
                    ImGui::EndDragDropSource();
                }

                // Drop target: entity (reparent) or asset (attach).
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(k_EntityDragDropId))
                    {
                        const auto* payload = static_cast<const EntityDragPayload*>(p->Data);
                        ReparentDropped(world, *payload, e);
                    }
                    if (const ImGuiPayload* ap = ImGui::AcceptDragDropPayload(k_AssetDragDropId))
                    {
                        const auto* assetPayload = static_cast<const AssetDragPayload*>(ap->Data);
                        if (m_OnAssetDropped)
                            m_OnAssetDropped(e, assetPayload->path, assetPayload->type);
                    }
                    ImGui::EndDragDropTarget();
                }

                RowContextMenu(world, e, selection, dirtyState);
            }

            if (open)
            {
                for (Entity child : GetChildren(world, e))
                    DrawEntityNode(world, child, selection, dirtyState);
                ImGui::TreePop();
            }
#endif
        }

      private:
#ifdef CADMIUM_IMGUI
        void RenderToolbar(World& world, EditorSelectionContext& selection,
                           const std::string& sceneName, EditorDirtyState& dirtyState)
        {
            if (ImGui::SmallButton("+ Create"))
                CreateEntity(world, selection, std::nullopt);

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::Text("Scene: %s", sceneName.c_str());

            if (dirtyState.IsDirty())
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.f));
                ImGui::TextUnformatted("*");
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Unsaved changes");
            }
        }

        void RenderEmptyAreaDropZone(World& world, EditorSelectionContext& selection)
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::InvisibleButton("##hierarchy_empty_area", ImVec2(avail.x, std::max(avail.y, 24.f)));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(k_EntityDragDropId))
                {
                    const auto* payload = static_cast<const EntityDragPayload*>(p->Data);
                    ReparentDropped(world, *payload, std::nullopt); // drop on empty space = unparent
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginPopupContextItem("ctx_hierarchy_empty"))
            {
                if (ImGui::MenuItem("Create Entity"))
                    CreateEntity(world, selection, std::nullopt);
                ImGui::EndPopup();
            }
        }

        void RowContextMenu(World& world, Entity e, EditorSelectionContext& selection, EditorDirtyState& dirtyState)
        {
            std::string popupId = "ctx_entity_" + std::to_string(EntityIndex(e));
            if (!ImGui::BeginPopupContextItem(popupId.c_str()))
                return;

            if (ImGui::MenuItem("Rename", "F2"))
                StartRename(world, e);

            if (ImGui::MenuItem("Create Child Entity"))
                CreateEntity(world, selection, e);

            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del"))
            {
                // Right-click outside multi-selection acts on just this row.
                if (!selection.IsSelected(e))
                    selection.SelectEntity(e);
                DeleteSelection(world, selection, dirtyState);
            }

            ImGui::EndPopup();
        }

        //  Actions

        void CreateEntity(World& world, EditorSelectionContext& selection, std::optional<Entity> parent)
        {
            auto cmd = std::make_unique<CreateEntityCommand>(world, "Entity", parent);
            CreateEntityCommand* raw = cmd.get();
            UndoStack::Get().ExecuteAndPush(std::move(cmd));
            if (auto created = raw->GetCreatedEntity())
                selection.SelectEntity(*created);
        }

        // Deliberately NOT routed through UndoStack: reversing a delete
        // would need to restore every component the entity had, not just
        // Tag/Parent, and there's no generic component snapshot/
        // serialization to do that yet
        void DeleteSelection(World& world, EditorSelectionContext& selection, EditorDirtyState& dirtyState)
        {
            for (Entity e : selection.GetMultiSelection())
                if (world.IsValid(e))
                    DestroySubtree(world, e);

            selection.ClearEntity();
            dirtyState.SetDirty();
        }

        void StartRename(World& world, Entity e)
        {
            m_RenamingEntity = e;
            m_JustStartedRename = true;
            std::string current = DisplayName(world, e);
            std::snprintf(m_RenameBuf, sizeof(m_RenameBuf), "%s", current.c_str());
        }

        void CommitRename(World& world, Entity e)
        {
            std::string newName(m_RenameBuf);
            std::optional<std::string> before = GetEntityName(world, e);
            std::optional<std::string> after = newName.empty() ? std::nullopt : std::optional<std::string>(newName);

            if (before != after)
                UndoStack::Get().ExecuteAndPush(std::make_unique<RenameEntityCommand>(world, e, before, after));

            m_RenamingEntity = k_NullEntity;
        }

        void ReparentDropped(World& world, const EntityDragPayload& payload, std::optional<Entity> newParent)
        {
            std::vector<std::unique_ptr<ICommand>> cmds;

            for (size_t i = 0; i < payload.count; ++i)
            {
                Entity e = payload.entities[i];
                if (!world.IsValid(e))
                    continue;
                if (newParent.has_value() && (*newParent == e || IsSelfOrDescendant(world, e, *newParent)))
                    continue; // no cycles or self-drop

                std::optional<Entity> before = GetEntityParent(world, e);
                if (before == newParent)
                    continue;

                cmds.push_back(std::make_unique<ReparentEntityCommand>(world, e, before, newParent));
            }

            if (cmds.empty())
                return;

            std::string desc = (cmds.size() == 1)
                ? std::string("Reparent Entity")
                : ("Reparent " + std::to_string(cmds.size()) + " Entities");

            UndoStack::Get().ExecuteAndPush(std::make_unique<CompositeCommand>(desc, std::move(cmds)));
        }

        void HandleShortcuts(World& world, EditorSelectionContext& selection, EditorDirtyState& dirtyState)
        {
            if (m_RenamingEntity != k_NullEntity)
                return; // don't interfere with rename input

            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                return;

            if (ImGui::IsKeyPressed(ImGuiKey_F2) && selection.GetMultiSelection().size() == 1)
                StartRename(world, selection.GetEntity());

            if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !selection.GetMultiSelection().empty())
                DeleteSelection(world, selection, dirtyState);
        }

        //  Helpers

        static std::string DisplayName(World& world, Entity e)
        {
            if (auto name = GetEntityName(world, e))
                return *name;
            return "Entity " + std::to_string(EntityIndex(e));
        }

        static std::vector<Entity> BuildFlatOrder(World& world)
        {
            std::vector<Entity> order;
            for (Entity e : world.AllEntities())
                if (!world.HasComponent<Parent>(e))
                    CollectSubtree(world, e, order);
            return order;
        }

        static std::vector<Entity> ComputeRange(const std::vector<Entity>& flat, Entity a, Entity b)
        {
            auto ita = std::find(flat.begin(), flat.end(), a);
            auto itb = std::find(flat.begin(), flat.end(), b);
            if (ita == flat.end() || itb == flat.end())
                return { b };

            size_t ia = static_cast<size_t>(std::distance(flat.begin(), ita));
            size_t ib = static_cast<size_t>(std::distance(flat.begin(), itb));
            if (ia > ib) std::swap(ia, ib);

            return std::vector<Entity>(flat.begin() + static_cast<long>(ia), flat.begin() + static_cast<long>(ib) + 1);
        }
#endif

        AssetDropCallback m_OnAssetDropped;

        Entity m_RenamingEntity{k_NullEntity};
        bool   m_JustStartedRename{false};
        char   m_RenameBuf[256]{};

        std::vector<Entity> m_FlatOrder; // for shift-click range select
    };
} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_HIERARCHY_PANEL
