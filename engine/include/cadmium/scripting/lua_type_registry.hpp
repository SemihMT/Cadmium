#ifndef CADMIUM_SCRIPTING_LUA_TYPE_REGISTRY_HPP
#define CADMIUM_SCRIPTING_LUA_TYPE_REGISTRY_HPP

#include <cadmium/ecs/lua_component.hpp>
#include <unordered_map>
#include <string>
#include <stdexcept>

namespace Cadmium
{

  class LuaTypeRegistry
  {
  public:
    // Called once per Lua component type at Component.Register time.
    // ID must already be assigned on the schema before calling this.
    void Register(LuaComponentSchema schema)
    {
      const std::string &name = schema.typeName;
      if (m_ByName.contains(name))
        throw std::runtime_error("Lua component type already registered: " + name);

      m_ByName[name] = std::move(schema);
    }

    const LuaComponentSchema *Find(const std::string &name) const
    {
      auto it = m_ByName.find(name);
      return it != m_ByName.end() ? &it->second : nullptr;
    }

    bool IsRegistered(const std::string &name) const
    {
      return m_ByName.contains(name);
    }

    // All registered schemas - useful for editor/debugger
    const std::unordered_map<std::string, LuaComponentSchema> &All() const
    {
      return m_ByName;
    }

  private:
    std::unordered_map<std::string, LuaComponentSchema> m_ByName;
  };

} // namespace Cadmium
#endif // CADMIUM_SCRIPTING_LUA_TYPE_REGISTRY_HPP
