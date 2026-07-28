#ifndef CADMIUM_EDITOR_REGISTER_BUILTIN_COMPONENTS
#define CADMIUM_EDITOR_REGISTER_BUILTIN_COMPONENTS

#include "cadmium/ecs/components.hpp"
#include <sol/forward.hpp>
namespace Cadmium::Editor::Detail
{
    void RegisterBuiltinComponents();
    void DrawExposedField(sol::environment& env,
                          const std::string& key,
                          sol::object value,
                          const FieldMetadata* meta);
    void DrawNumberField(sol::environment& env,
                         const std::string& key,
                         float current,
                         const FieldMetadata* meta);
    void DrawStringField(sol::environment& env,
                         const std::string& key,
                         std::string current,
                         const FieldMetadata* meta);
    void DrawBoolField(sol::environment& env,
                       const std::string& key,
                       bool current,
                       const FieldMetadata* meta);
    void DrawTableField(sol::environment& env,
                        const std::string& key,
                        sol::table table,
                        const FieldMetadata* meta);
    void DrawTooltip(const FieldMetadata* meta);
} // namespace Cadmium::Editor::Detail

#endif
