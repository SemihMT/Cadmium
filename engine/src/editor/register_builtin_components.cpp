#include <cadmium/ecs/components.hpp>
#include <cadmium/editor/component_inspector.hpp>
#include <cadmium/editor/register_builtin_components.hpp>
#include <imgui.h>

namespace Cadmium::Editor::Detail
{
    void RegisterBuiltinComponents()
    {
        static bool done = false;
        if (done)
            return;
        done = true;

        auto& inspector = ComponentInspector::Get();

        inspector.Register<Transform>(
            "Transform",
            [](Transform& t)
            {
                ImGui::Text("Position: %.2f, %.2f, %.2f", t.position.x, t.position.y, t.position.z);
                ImGui::Text("Rotation Z: %.1f", t.GetRotationZ());
                ImGui::Text("Scale: %.2f, %.2f, %.2f", t.scale.x, t.scale.y, t.scale.z);
            });

        inspector.Register<Velocity>(
            "Velocity", [](Velocity& v) { ImGui::Text("Velocity: %.2f, %.2f", v.x, v.y); });

        inspector.Register<Tag>("Tag", [](Tag& t) { ImGui::Text("Name: %s", t.name.c_str()); });

        inspector.Register<Parent>(
            "Parent", [](Parent& p) { ImGui::Text("Parent entity: %u", EntityIndex(p.entity)); });

        inspector.Register<Script>("Script",
                                   [](Script& s)
                                   {
                                       for (auto& instance : s.instances)
                                       {
                                           if (!instance.env.valid())
                                           {
                                               ImGui::TextDisabled("(no data)");
                                               continue;
                                           }

                                           if (!ImGui::TreeNode(instance.name.c_str()))
                                               continue;

                                           for (const std::string& key : instance.fieldOrder)
                                           {
                                               sol::object value = instance.env[key];

                                               auto metaIt = instance.fieldMetadata.find(key);
                                               const FieldMetadata* meta =
                                                   (metaIt != instance.fieldMetadata.end())
                                                   ? &metaIt->second
                                                   : nullptr;

                                               DrawExposedField(instance.env, key, value, meta);
                                           }

                                           ImGui::TreePop();
                                       }
                                   });
    }

    void DrawExposedField(sol::environment& env,
                          const std::string& key,
                          sol::object value,
                          const FieldMetadata* meta)
    {
        switch (value.get_type())
        {
        case sol::type::number:
            DrawNumberField(env, key, value.as<float>(), meta);
            break;

        case sol::type::string:
            DrawStringField(env, key, value.as<std::string>(), meta);
            break;

        case sol::type::boolean:
            DrawBoolField(env, key, value.as<bool>(), meta);
            break;

        case sol::type::table:
            DrawTableField(env, key, value.as<sol::table>(), meta);
            break;

        default:
            ImGui::TextDisabled("%s: unsupported type", key.c_str());
            break;
        }
    }
    void DrawNumberField(sol::environment& env,
                         const std::string& key,
                         float current,
                         const FieldMetadata* meta)
    {
        bool changed = false;

        const float step = (meta && meta->step) ? static_cast<float>(*meta->step) : 1.0f;

        const bool hasRange = meta && meta->min && meta->max;

        std::string_view widget;

        if (meta && meta->widget)
            widget = *meta->widget;
        // Validate incompatible widget/type combinations.
        if (widget == "color")
        {
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                               "%s: Widget(\"color\") requires a table.",
                               key.c_str());

            DrawTooltip(meta);
            return;
        }

        if (widget == "slider")
        {
            if (!hasRange)
            {
                ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                                   "%s: Slider requires Range(min, max).",
                                   key.c_str());

                DrawTooltip(meta);
                return;
            }

            changed = ImGui::SliderFloat(key.c_str(), &current, *meta->min, *meta->max);

            // Optional snapping.
            if (changed && meta->step)
            {
                current = std::round(current / *meta->step) * *meta->step;
            }
        }
        else
        {
            if (hasRange)
            {
                changed = ImGui::DragFloat(key.c_str(), &current, step, *meta->min, *meta->max);
            }
            else
            {
                changed = ImGui::DragFloat(key.c_str(), &current, step);
            }
        }

        DrawTooltip(meta);

        if (changed)
            env[key] = current;
    }

    void DrawStringField(sol::environment& env,
                         const std::string& key,
                         std::string current,
                         const FieldMetadata* meta)
    {
        char buffer[256];
        std::strncpy(buffer, current.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer)))
            env[key] = std::string(buffer);

        DrawTooltip(meta);
    }

    void DrawBoolField(sol::environment& env,
                       const std::string& key,
                       bool current,
                       const FieldMetadata* meta)
    {
        if (ImGui::Checkbox(key.c_str(), &current))
            env[key] = current;

        DrawTooltip(meta);
    }

    void DrawTableField(sol::environment& env,
                        const std::string& key,
                        sol::table table,
                        const FieldMetadata* meta)
    {
        std::string_view widget;

        if (meta && meta->widget)
            widget = *meta->widget;

        if (widget == "color")
        {
            float rgb[3] = {table.get_or(1, 0.0f), table.get_or(2, 0.0f), table.get_or(3, 0.0f)};

            if (ImGui::ColorEdit3(key.c_str(), rgb))
            {
                table[1] = rgb[0];
                table[2] = rgb[1];
                table[3] = rgb[2];
            }

            DrawTooltip(meta);
            return;
        }

        ImGui::TextDisabled("%s: (table)", key.c_str());
    }
    void DrawTooltip(const FieldMetadata* meta)
    {
        if (meta && meta->tooltip && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", meta->tooltip->c_str());
        }
    }
} // namespace Cadmium::Editor::Detail
