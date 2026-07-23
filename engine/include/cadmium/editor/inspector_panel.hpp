#ifndef CADMIUM_EDITOR_INSPECTOR_PANEL
#define CADMIUM_EDITOR_INSPECTOR_PANEL
#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/component_inspector.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#endif

namespace Cadmium::Editor
{
  class InspectorPanel
  {
  public:
    void Render(World& world, Entity selected, const char* title = "Inspector")
    {
#ifdef CADMIUM_IMGUI
      ImGui::Begin(title);
      if (selected == k_NullEntity || !world.IsValid(selected))
        ImGui::TextDisabled("No entity selected");
      else
        for (auto& entry : ComponentInspector::Get().Entries())
          if (entry.has(world, selected) &&
              ImGui::CollapsingHeader(entry.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            entry.draw(world, selected);
      ImGui::End();
#endif
    }
  };
} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_INSPECTOR_PANEL
