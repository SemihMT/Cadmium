#ifndef CADMIUM_ECS_LUA_COMPONENT_HPP
#define CADMIUM_ECS_LUA_COMPONENT_HPP
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <cadmium/ecs/component_id.hpp>

namespace Cadmium
{
  // Lua components can only be composed of the following types
  using LuaFieldValue = std::variant<float, bool, std::string>;

  enum class LuaFieldType
  {
    Float,
    Bool,
    String
  };

  struct LuaFieldDef
  {
    LuaFieldType type;
    LuaFieldValue defaultValue;
  };

  struct LuaComponentSchema
  {
    ComponentID id;
    std::string typeName;
    std::vector<std::string> fieldOrder;
    std::unordered_map<std::string, LuaFieldDef> fields;
  };

  struct LuaComponentData
  {
    const LuaComponentSchema *schema; // non-owning
    std::unordered_map<std::string, LuaFieldValue> values;

    static LuaComponentData FromSchema(const LuaComponentSchema &s)
    {
      LuaComponentData d;
      d.schema = &s;
      for (const auto &name : s.fieldOrder)
        d.values[name] = s.fields.at(name).defaultValue;
      return d;
    }
  };

} // namespace Cadmium
#endif // CADMIUM_ECS_LUA_COMPONENT_HPP
