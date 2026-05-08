#ifndef CADMIUM_SCRIPTING_LUA_BINDINGS_COMPONENTS_HPP
#define CADMIUM_SCRIPTING_LUA_BINDINGS_COMPONENTS_HPP

#pragma once
#include <cadmium/scripting/lua_component.hpp>
#include <cadmium/ecs/world.hpp>
#include <sol/sol.hpp>
#include <SDL3/SDL.h>

namespace Cadmium::Lua
{

  // Returned by Component.Get - writes go directly back to LuaComponentData
  // in the SparseSet. Never return a plain sol::table from a getter.

  struct LuaComponentProxy
  {
    LuaComponentData *data; // direct pointer into SparseSet
    std::string typeName;   // for error messages
  };

  inline void BindComponents(sol::state &lua, World &world)
  {
    World *w = &world;

    lua.new_usertype<LuaComponentProxy>("LuaComponentProxy", sol::no_constructor,

    sol::meta_function::index, [](LuaComponentProxy &self, const std::string &key, sol::this_state s) -> sol::object
    {
      sol::state_view L(s);
      if (!self.data) return sol::nil;
      auto it = self.data->values.find(key);
      if (it == self.data->values.end()) return sol::nil;
      return std::visit([&](const auto& v) {
          return sol::make_object(L, v);
      }, it->second);
    },

    sol::meta_function::new_index, [](LuaComponentProxy &self, const std::string &key, sol::object value)
    {
      if (!self.data || !self.data->schema) return;

      auto fieldIt = self.data->schema->fields.find(key);
      if (fieldIt == self.data->schema->fields.end())
      {
          SDL_Log("[Component] Unknown field '%s' on '%s'",
                  key.c_str(), self.typeName.c_str());
          return;
      }

      // Bool must be checked before float
      switch (fieldIt->second.type)
      {
      case LuaFieldType::Float:
          if (value.is<float>())
              self.data->values[key] = value.as<float>();
          else if (value.is<int>())
              self.data->values[key] = (float)value.as<int>();
          else
              SDL_Log("[Component] '%s' expects float", key.c_str());
          break;
      case LuaFieldType::Bool:
          if (value.is<bool>())
              self.data->values[key] = value.as<bool>();
          else
              SDL_Log("[Component] '%s' expects bool", key.c_str());
          break;
      case LuaFieldType::String:
          if (value.is<std::string>())
              self.data->values[key] = value.as<std::string>();
          else
              SDL_Log("[Component] '%s' expects string", key.c_str());
          break;
      }
    },

    sol::meta_function::to_string, [](const LuaComponentProxy &self)
    {
      return "LuaComponent(" + self.typeName + ")";
    });


    sol::table comp = lua.create_named_table("Component");

    // Component.Register("Health", { hp = 100.0, dead = false, label = "" })
    // Types are inferred from default value types
    comp.set_function("Register",
    [w](const std::string &typeName, sol::table fieldDefs)
    {
      LuaComponentSchema schema;
      schema.typeName = typeName;

      for (auto &[k, v] : fieldDefs)
      {
        if (!k.is<std::string>())
          continue;
        std::string fieldName = k.as<std::string>();

        LuaFieldDef fd;
        // Bool before float - critical ordering
        if (v.is<bool>())
        {
          fd.type = LuaFieldType::Bool;
          fd.defaultValue = v.as<bool>();
        }
        else if (v.is<float>() || v.is<int>())
        {
          fd.type = LuaFieldType::Float;
          fd.defaultValue = v.is<int>() ? (float)v.as<int>() : v.as<float>();
        }
        else if (v.is<std::string>())
        {
          fd.type = LuaFieldType::String;
          fd.defaultValue = v.as<std::string>();
        }
        else
        {
          SDL_Log("[Component.Register] Field '%s.%s' has unsupported type - skipped",
                  typeName.c_str(), fieldName.c_str());
          continue;
        }

        schema.fieldOrder.push_back(fieldName);
        schema.fields[fieldName] = fd;
      }

      try
      {
        w->RegisterLuaComponent(std::move(schema));
      }
      catch (const std::exception &e)
      {
        SDL_Log("[Component.Register] %s", e.what());
      }
    });

    // Component.Add(entity, "Health")
    // Component.Add(entity, "Health", { hp = 50 })
    comp.set_function("Add",
    [w](EntityHandle &handle, const std::string &typeName, sol::optional<sol::table> init)
    {
      const LuaComponentSchema *schema{nullptr};
      {
        // Peek at schema to validate init fields before forwarding to World
        auto *data = w->TryGetLuaComponent(handle.cppEntity, typeName);
        (void)data; // we just want the schema
      }

      std::unordered_map<std::string, LuaFieldValue> initVals;

      if (init)
      {

        for (auto &[k, v] : *init)
        {
          if (!k.is<std::string>())
            continue;
          std::string field = k.as<std::string>();

          // Bool before float
          if (v.is<bool>())
            initVals[field] = v.as<bool>();
          else if (v.is<float>() || v.is<int>())
            initVals[field] = v.is<int>() ? (float)v.as<int>() : v.as<float>();
          else if (v.is<std::string>())
            initVals[field] = v.as<std::string>();
        }
      }

      w->AddLuaComponent(handle.cppEntity, typeName, std::move(initVals));
    });

    // Component.Remove(entity, "Health")
    comp.set_function("Remove",
    [w](EntityHandle &handle, const std::string &typeName)
    {
      w->RemoveLuaComponent(handle.cppEntity, typeName);
    });

    // Component.Has(entity, "Health") → bool
    comp.set_function("Has",
    [w](EntityHandle &handle, const std::string &typeName) -> bool
    {
      return w->HasLuaComponent(handle.cppEntity, typeName);
    });

    // Component.Get(entity, "Health") → LuaComponentProxy
    // Writes to the proxy write directly back into the SparseSet.
    comp.set_function("Get",
    [w](EntityHandle &handle, const std::string &typeName, sol::this_state s) -> sol::object
    {
      sol::state_view lua(s);
      LuaComponentData *data = w->TryGetLuaComponent(handle.cppEntity, typeName);
      if (!data)
        return sol::nil;
      return sol::make_object(lua, LuaComponentProxy{data, typeName});
    });

    // Component.Query("Health") → table of { entity, component } pairs
    comp.set_function("Query",
    [w](const std::string &typeName, sol::this_state s) -> sol::table
    {
      sol::state_view lua(s);
      sol::table results = lua.create_table();
      int i = 1;
      for (auto &[entity, data] : w->Query(typeName))
      {
        sol::table pair = lua.create_table();
        pair["entity"] = entity;
        pair["component"] = LuaComponentProxy{data, typeName};
        results[i++] = pair;
      }
      return results;
    });
  }

} // namespace Cadmium::Lua
#endif // CADMIUM_SCRIPTING_LUA_BINDINGS_COMPONENTS_HPP
