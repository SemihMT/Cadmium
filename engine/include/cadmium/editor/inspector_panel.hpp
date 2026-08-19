#ifndef CADMIUM_EDITOR_INSPECTOR_PANEL
#define CADMIUM_EDITOR_INSPECTOR_PANEL
#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/asset_drag_payload.hpp>
#include <cadmium/editor/component_commands.hpp>
#include <cadmium/editor/component_inspector.hpp>
#include <cadmium/editor/editor_selection.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <functional>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Cadmium::Editor
{
  class InspectorPanel
  {
  public:
    // Called when a Script asset is dropped anywhere in the Inspector.
    using ScriptDropCallback = std::function<void(World&, Entity, const std::string& scriptPath)>;

    void SetOnScriptDropped(ScriptDropCallback cb) { m_OnScriptDropped = std::move(cb); }

    // selection: reads the entity currently selected in the Hierarchy
    // open: optional visibility toggle
    void Render(World& world, const EditorSelectionContext& selection,
                const char* title = "Inspector", bool* open = nullptr)
    {
#ifdef CADMIUM_IMGUI
      if (open && !*open)
        return;

      if (!ImGui::Begin(title, open))
      {
        ImGui::End();
        return;
      }

      // Whole-window drop target for Script assets, so a script can be
      // attached even when the entity has no Script component yet
      if (ImGui::BeginDragDropTarget())
      {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(k_AssetDragDropId))
        {
          const auto* payload = static_cast<const AssetDragPayload*>(p->Data);
          HandleAssetDrop(world, selection, *payload);
        }
        ImGui::EndDragDropTarget();
      }

      Entity selected = selection.GetEntity();
      size_t multiCount = selection.GetMultiSelection().size();

      if (multiCount > 1)
      {
        ImGui::Text("%zu entities selected", multiCount);
        ImGui::TextDisabled("Multi-entity editing isn't supported yet -");
        ImGui::TextDisabled("select a single entity to edit components.");
        ImGui::End();
        return;
      }

      if (selected == k_NullEntity || !world.IsValid(selected))
      {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
      }

      for (auto& entry : ComponentInspector::Get().Entries())
      {
        if (!entry.has(world, selected))
          continue;

        bool isRequired = IsAlwaysPresentComponent(entry.name);
        bool visible = true;
        bool headerOpen = isRequired
            ? ImGui::CollapsingHeader(entry.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen) // no "x" - can't be removed
            : ImGui::CollapsingHeader(entry.name.c_str(), &visible, ImGuiTreeNodeFlags_DefaultOpen);

        // Per-component drop target
        if (ImGui::BeginDragDropTarget())
        {
          if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(k_AssetDragDropId))
          {
            const auto* payload = static_cast<const AssetDragPayload*>(p->Data);
            if (entry.onAssetDropped)
              entry.onAssetDropped(world, selected, payload->path, payload->type);
          }
          ImGui::EndDragDropTarget();
        }

        if (!visible)
        {
          // "x" button was clicked.
          entry.remove(world, selected);
          continue;
        }

        if (headerOpen)
          entry.draw(world, selected);
      }

      ImGui::Spacing();
      ImGui::Separator();

      RenderAddComponentMenu(world, selected);

      ImGui::End();
#endif
    }

  private:
#ifdef CADMIUM_IMGUI
    void RenderAddComponentMenu(World& world, Entity selected)
    {
      if (ImGui::Button("+ Add Component"))
        ImGui::OpenPopup("##AddComponentPopup");

      if (ImGui::BeginPopup("##AddComponentPopup"))
      {
        bool anyAvailable = false;

        for (auto& entry : ComponentInspector::Get().Entries())
        {
          if (entry.has(world, selected))
            continue; // already on this entity
          if (IsAlwaysPresentComponent(entry.name))
            continue; // guaranteed present at creation

          anyAvailable = true;
          if (ImGui::MenuItem(entry.name.c_str()))
            entry.addDefault(world, selected);
        }

        if (!anyAvailable)
          ImGui::TextDisabled("All available components added");

        ImGui::EndPopup();
      }
    }

    void HandleAssetDrop(World& world, const EditorSelectionContext& selection, const AssetDragPayload& payload)
    {
      Entity selected = selection.GetEntity();
      if (selected == k_NullEntity || !world.IsValid(selected))
        return;

      if (payload.type != AssetType::Script)
        return; // Transform/Velocity/Tag have no asset-backed field to receive a drop

      if (!world.HasComponent<Script>(selected))
        UndoStack::Get().ExecuteAndPush(std::make_unique<AddComponentCommand<Script>>(world, selected, "Script"));

      if (m_OnScriptDropped)
        m_OnScriptDropped(world, selected, payload.path);
    }
#endif

    ScriptDropCallback m_OnScriptDropped;
  };
} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_INSPECTOR_PANEL
