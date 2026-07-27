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
      for (const auto& instance : s.instances)
      {
          if (!instance.env.valid())
          {
              ImGui::TextDisabled("(no data)");
              continue;
          }

          if (ImGui::TreeNode(instance.name.c_str()))
          {
              for (auto& [key, value] : instance.env)
              {
                  std::string keyStr = key.as<std::string>();
                  if (keyStr == "self")
                      continue;

                  if (value.get_type() == sol::type::number)
                      ImGui::Text("%s: %.2f", keyStr.c_str(), value.as<double>());
                  else if (value.get_type() == sol::type::string)
                      ImGui::Text("%s: %s", keyStr.c_str(), value.as<std::string>().c_str());
                  else if (value.get_type() == sol::type::boolean)
                      ImGui::Text("%s: %s", keyStr.c_str(), value.as<bool>() ? "true" : "false");
              }
              ImGui::TreePop();
          }
      }
    });
  }
}
