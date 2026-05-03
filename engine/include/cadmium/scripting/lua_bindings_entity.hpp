#ifndef CADMIUM_LUA_BINDINGS_ENTITY_HPP
#define CADMIUM_LUA_BINDINGS_ENTITY_HPP
#include <cadmium/scripting/entity_registry.hpp>
#include <cadmium/ecs/registry.hpp>
#include <cadmium/ecs/components.hpp>
#include <sol/sol.hpp>
#include <string>
#include <unordered_set>

namespace Cadmium::Lua
{

  // Built-in property names that proxy directly to the C++ Transform component.
  // Everything else falls through to the Lua table.
  static const std::unordered_set<std::string> k_TransformFields = {
      // Position
      "x", "y", "z",
      // Rotation
      "rotationX", "rotationY", "rotationZ",
      "rotation", // alias for rotationZ , for 2D use
      // Scale
      "scaleX", "scaleY", "scaleZ"};

  // Fields that are engine-managed and should not be written to the fallback table
  static const std::unordered_set<std::string> k_EngineFields = {
      "x", "y", "z",
      "rotationX", "rotationY", "rotationZ", "rotation",
      "scaleX", "scaleY", "scaleZ",
      "visible", "active", "name", "tag"};

  // ── EntityHandle ─────────────────────────────────────────────────────────
  // Userdata that wraps a Lua table (plain data) and a C++ ECS entity ID.
  // __index and __newindex intercept built-in property names and route them
  // to the C++ Transform. Everything else goes to/from the Lua table.
  struct EntityHandle
  {
    sol::table table;   // backing Lua table for plain fields and hooks
    Entity cppEntity;   // C++ ECS entity ID
    Registry *registry; // pointer to scene's ECS registry

    // ── Transform proxy helpers ───────────────────────────────────────────

    Transform *GetTransform() const
    {
      if (!registry || !registry->IsValid(cppEntity))
        return nullptr;
      return registry->TryGetComponent<Transform>(cppEntity);
    }

    float GetField(const std::string &key) const
    {
      Transform* t = GetTransform();
      if (!t) return 0.f;

      if (key == "x")         return t->GetX();
      if (key == "y")         return t->GetY();
      if (key == "z")         return t->GetZ();
      if (key == "rotationX") return t->GetRotationX();
      if (key == "rotationY") return t->GetRotationY();
      if (key == "rotationZ") return t->GetRotationZ();
      if (key == "rotation")  return t->GetRotation();   // alias
      if (key == "scaleX")    return t->GetScaleX();
      if (key == "scaleY")    return t->GetScaleY();
      if (key == "scaleZ")    return t->GetScaleZ();
      return 0.f;
    }

    void SetField(const std::string &key, float value)
    {
      Transform* t = GetTransform();
      if (!t) return;

      if (key == "x")         { t->SetX(value);         return; }
      if (key == "y")         { t->SetY(value);         return; }
      if (key == "z")         { t->SetZ(value);         return; }
      if (key == "rotationX") { t->SetRotationX(value); return; }
      if (key == "rotationY") { t->SetRotationY(value); return; }
      if (key == "rotationZ") { t->SetRotationZ(value); return; }
      if (key == "rotation")  { t->SetRotation(value);  return; }
      if (key == "scaleX")    { t->SetScaleX(value);    return; }
      if (key == "scaleY")    { t->SetScaleY(value);    return; }
      if (key == "scaleZ")    { t->SetScaleZ(value);    return; }
    }
  };

  // ── BindEntity ────────────────────────────────────────────────────────────
  inline void BindEntity(sol::state &lua,
                        EntityRegistry &luaRegistry,
                        Registry &ecsRegistry)
  {
    sol::state *L = &lua;
    EntityRegistry *luaReg = &luaRegistry;
    Registry *ecsReg = &ecsRegistry;

    // ── EntityHandle usertype ─────────────────────────────────────────────
    lua.new_usertype<EntityHandle>("EntityHandle", sol::no_constructor,

    // __index: intercept built-in fields, fall through to table for rest
    sol::meta_function::index, [](EntityHandle &self, const std::string &key, sol::this_state s) -> sol::object
    {
      sol::state_view lua(s);

      // Transform proxy fields
      if (k_TransformFields.count(key))
      {
          Transform* t = self.GetTransform();
          if (!t)
            return sol::make_object(lua, 0.f);
          return sol::make_object(lua, self.GetField(key));
      }
      // Everything else - backing table
      return self.table[key];
    },

    // __newindex: intercept built-in fields, fall through to table for rest
    sol::meta_function::new_index, [](EntityHandle &self, const std::string &key, sol::object value)
    {
      // Transform fields - always proxy to C++, never touch backing table
      if (k_TransformFields.count(key))
      {
        float v = 0.f;
        if (value.is<float>())
          v = value.as<float>();
        else if (value.is<int>())
          v = (float)value.as<int>();
        else
        {
          // Wrong type for a transform field - log and ignore
          SDL_Log("[Entity] '%s' requires a number, got %s",
                  key.c_str(), lua_typename(value.lua_state(), (int)value.get_type()));
          return;
        }
        self.SetField(key, v);
        return;
      }

      // Engine-managed fields - do not write to fallback table
      if (k_EngineFields.count(key))
      {
        return;
      }

      // Everything else - backing table
      self.table[key] = value;
    },

    // __tostring for debugging
    sol::meta_function::to_string, [](EntityHandle &self) -> std::string
    {
      sol::object name = self.table["name"];
      if (name.is<std::string>())
          return "Entity(" + name.as<std::string>() + ")";
      return "Entity";
    },

    // __eq for identity comparison (Entity.Destroy(e), e == other, etc.)
    sol::meta_function::equal_to, [](const EntityHandle &a, const EntityHandle &b)
    {
      return a.cppEntity == b.cppEntity;
    });

    // ── Entity table ──────────────────────────────────────────────────────
    sol::table entity = lua.create_named_table("Entity");

    // Entity.New(name?) → EntityHandle
    entity.set_function("New",
    [L, luaReg, ecsReg](sol::variadic_args args) -> EntityHandle
    {
      // Create C++ ECS entity with Transform
      Entity cppEntity = ecsReg->CreateEntity();
      ecsReg->AddComponent<Transform>(cppEntity, Transform{});

      // Create backing Lua table for plain fields
      sol::table table = L->create_table();

      // Set defaults
      table["visible"] = true;
      table["active"] = true;
      table["name"] = (args.size() > 0 && args[0].is<std::string>())
                          ? args[0].get<std::string>()
                          : "";
      table["tag"] = "";

      // Build the handle
      EntityHandle handle{table, cppEntity, ecsReg};

      // Wrap the handle as a sol object - this is what gets passed
      // as `self` to all hooks so __index/__newindex fire correctly
      sol::object handleObj = sol::make_object(*L, handle);

      // Register both with the registry
      luaReg->Register(table, handleObj);

      // Call OnAttach with the handle as self, if defined
      // Note: user sets OnAttach after New() returns in normal usage,
      // so this only fires for template-constructed entities
      sol::protected_function onAttach = table["OnAttach"];
      if (onAttach.valid())
        onAttach(handleObj);

      return handle;
    });

    // Entity.Destroy(entity)
    // Accepts the EntityHandle userdata returned by Entity.New()
    entity.set_function("Destroy",
    [luaReg](EntityHandle &handle)
    {
      // Mark for deferred removal - actual erase happens in FlushDestroyed
      luaReg->MarkForDestroy(handle.table);
      // Immediately inactive so hooks are skipped this frame
      handle.table["active"] = false;
    });

    // Entity.Find("name") → EntityHandle or nil
    entity.set_function("Find",
    [luaReg, L](const std::string &name) -> sol::object
    {
      return luaReg->Find(name, sol::state_view(*L));
    });

    // Entity.FindAll("tag") → table of EntityHandles
    entity.set_function("FindAll",
    [luaReg, L](const std::string &tag) -> sol::table
    {
      return luaReg->FindAll(tag, sol::state_view(*L));
    });

    // Entity.FindNear(x, y, radius) → table of EntityHandles
    entity.set_function("FindNear",
    [luaReg, L](float x, float y, float radius) -> sol::table
    {
      return luaReg->FindNear(x, y, radius, sol::state_view(*L));
    });

    // Entity.Count() → int
    entity.set_function("Count",
    [luaReg]() -> int
    {
      return luaReg->Count();
    });
  }
}// namespace Cadmium::Lua

#endif // CADMIUM_LUA_BINDINGS_ENTITY_HPP
