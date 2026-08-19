#ifndef CADMIUM_EDITOR_VIEWPORT_PANEL_HPP
#define CADMIUM_EDITOR_VIEWPORT_PANEL_HPP

#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/component_edit_command.hpp>
#include <cadmium/editor/render_viewport.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <cadmium/render/renderer.hpp>
#include <cmath>
#include <functional>
#include <memory>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Cadmium::Editor
{

  class ViewportPanel
  {
  public:
    using ResizeCallback = std::function<void(int w, int h)>;

    explicit ViewportPanel(ResizeCallback onResize)
        : m_OnResize(std::move(onResize)) {}

    // world/selected/renderer: needed for the position gizmo drawn over
    // the selected entity, if any (see RenderPositionGizmo). Pass
    // selected == k_NullEntity to skip gizmo rendering entirely (e.g. no
    // Scene loaded yet).
    // open: optional visibility toggle, driven by the editor's Window menu.
    //       Pass nullptr to always render (original behaviour, no close button).
    void Render(RenderViewport &viewport, World* world, Entity selected, IRenderer* renderer,
                const char *title = "Viewport", bool* open = nullptr)
    {
#ifdef CADMIUM_IMGUI
      if (open && !*open)
        return;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.f, 0.f});
      bool isOpen = ImGui::Begin(title, open);
      ImGui::PopStyleVar();

      if (isOpen)
      {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        int w = std::max(1, static_cast<int>(avail.x));
        int h = std::max(1, static_cast<int>(avail.y));

        if (w != m_LastW || h != m_LastH)
        {
          m_LastW = w;
          m_LastH = h;
          if (m_OnResize)
            m_OnResize(w, h);
        }

        if (viewport.IsReady())
        {
          ImVec2 imgPos = ImGui::GetCursorScreenPos();
          ImGui::Image(viewport.GetImTextureID(), avail);
          viewport.SetScreenPos(imgPos.x, imgPos.y);

          if (world && renderer && selected != k_NullEntity && world->IsValid(selected) &&
              world->HasComponent<Transform>(selected))
            RenderPositionGizmo(viewport, *world, selected, *renderer);
          else if (m_Dragging)
            m_Dragging = false; // selection changed mid-drag
        }
        else
        {
          ImGui::TextDisabled("Viewport not available");
        }
      }
      ImGui::End();
#else
      (void)viewport; (void)world; (void)selected; (void)renderer; (void)title; (void)open;
#endif
    }

  private:
#ifdef CADMIUM_IMGUI
    // Position-only gizmo
    void RenderPositionGizmo(RenderViewport& viewport, World& world, Entity selected, IRenderer& renderer)
    {
      Transform& transform = world.GetComponent<Transform>(selected);

      float localX, localY;
      renderer.WorldToScreen(transform.position.x, transform.position.y, localX, localY);
      ImVec2 handleScreen(viewport.GetScreenX() + localX, viewport.GetScreenY() + localY);

      ImVec2 mouse = ImGui::GetIO().MousePos;
      float dist = std::hypot(mouse.x - handleScreen.x, mouse.y - handleScreen.y);
      bool hovered = !m_Dragging && dist < k_HandleRadius && ImGui::IsWindowHovered();

      auto* drawList = ImGui::GetWindowDrawList();
      ImU32 fill = (m_Dragging || hovered) ? IM_COL32(255, 210, 60, 255) : IM_COL32(255, 255, 255, 230);
      drawList->AddCircleFilled(handleScreen, k_HandleRadius, fill);
      drawList->AddCircle(handleScreen, k_HandleRadius, IM_COL32(20, 20, 20, 200), 0, 2.0f);

      if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      {
        m_Dragging = true;
        m_DragEntity = selected;
        m_DragSnapshot = transform;
      }

      if (m_Dragging && m_DragEntity == selected)
      {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
          float localMouseX = mouse.x - viewport.GetScreenX();
          float localMouseY = mouse.y - viewport.GetScreenY();
          float worldX, worldY;
          renderer.ScreenToWorld(localMouseX, localMouseY, worldX, worldY);
          transform.position.x = worldX;
          transform.position.y = worldY;
        }
        else
        {
          m_Dragging = false;
          if (transform.position != m_DragSnapshot.position)
            UndoStack::Get().Push(std::make_unique<ComponentEditCommand<Transform>>(
                world, selected, "Transform", m_DragSnapshot, transform));
        }
      }
    }

    static constexpr float k_HandleRadius = 6.0f;

    bool      m_Dragging{false};
    Entity    m_DragEntity{k_NullEntity};
    Transform m_DragSnapshot{};
#endif

    ResizeCallback m_OnResize;
    int m_LastW = 0;
    int m_LastH = 0;
  };

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_VIEWPORT_PANEL_HPP
