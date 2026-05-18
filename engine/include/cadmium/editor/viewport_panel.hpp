#ifndef CADMIUM_EDITOR_VIEWPORT_PANEL_HPP
#define CADMIUM_EDITOR_VIEWPORT_PANEL_HPP

#include <cadmium/editor/render_viewport.hpp>
#include <functional>

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

    void Render(const RenderViewport &viewport, const char *title = "Viewport")
    {
#ifdef CADMIUM_IMGUI
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.f, 0.f});
      bool open = ImGui::Begin(title);
      ImGui::PopStyleVar();

      if (open)
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
          ImGui::Image(viewport.GetImTextureID(), avail);
        }
        else
        {
          ImGui::TextDisabled("Viewport not available");
        }
      }
      ImGui::End();
#else
      (void)viewport;
      (void)title;
#endif
    }

  private:
    ResizeCallback m_OnResize;
    int m_LastW = 0;
    int m_LastH = 0;
  };

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_VIEWPORT_PANEL_HPP
