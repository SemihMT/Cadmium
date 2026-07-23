#include <cadmium/editor/register_builtin_components.hpp>
#include <cadmium/editor/component_inspector.hpp>
#include <cadmium/ecs/components.hpp>
#include <imgui.h>

namespace Cadmium::Editor::Detail
{
  void RegisterBuiltinComponents()
  {
    static bool done = false;
    if (done) return;
    done = true;

    auto& inspector = ComponentInspector::Get();

    inspector.Register<Transform>("Transform", [](Transform& t)
    {
      ImGui::Text("Position: %.2f, %.2f, %.2f", t.position.x, t.position.y, t.position.z);
      ImGui::Text("Rotation Z: %.1f", t.GetRotationZ());
      ImGui::Text("Scale: %.2f, %.2f, %.2f", t.scale.x, t.scale.y, t.scale.z);
    });

    inspector.Register<Velocity>("Velocity", [](Velocity& v)
    {
      ImGui::Text("Velocity: %.2f, %.2f", v.x, v.y);
    });

    inspector.Register<Tag>("Tag", [](Tag& t)
    {
      ImGui::Text("Name: %s", t.name.c_str());
    });

    inspector.Register<Parent>("Parent", [](Parent& p)
    {
      ImGui::Text("Parent entity: %u", EntityIndex(p.entity));
    });

    inspector.Register<Script>("Script", [](Script& s)
    {
      if (!s.self.valid())
        ImGui::TextDisabled("(no data)");
      else
        ImGui::TextDisabled("Lua entity (table contents not shown yet)");
    });
  }
}
