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

  static const std::unordered_set<std::string> k_TransformFields = {
      "x", "y", "z",
      "rotationX", "rotationY", "rotationZ",
      "rotation",
      "scaleX", "scaleY", "scaleZ"};

  static const std::unordered_set<std::string> k_EngineFields = {
      "x", "y", "z",
      "rotationX", "rotationY", "rotationZ", "rotation",
      "scaleX", "scaleY", "scaleZ",
      // "visible", "active", "name", "tag"
      };

  struct EntityHandle
  {
    sol::table table;
    Entity     cppEntity;
    Registry*  registry;

    Transform* GetTransform() const
    {
      if (!registry || !registry->IsValid(cppEntity))
        return nullptr;
      return registry->TryGetComponent<Transform>(cppEntity);
    }

    float GetField(const std::string& key) const
    {
      Transform* t = GetTransform();
      if (!t) return 0.f;
      if (key == "x")         return t->GetX();
      if (key == "y")         return t->GetY();
      if (key == "z")         return t->GetZ();
      if (key == "rotationX") return t->GetRotationX();
      if (key == "rotationY") return t->GetRotationY();
      if (key == "rotationZ") return t->GetRotationZ();
      if (key == "rotation")  return t->GetRotation();
      if (key == "scaleX")    return t->GetScaleX();
      if (key == "scaleY")    return t->GetScaleY();
      if (key == "scaleZ")    return t->GetScaleZ();
      return 0.f;
    }

    void SetField(const std::string& key, float value)
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

  //  Defined in lua_bindings.cpp
  // Call once at engine startup on the sol::state.
  // Registers the EntityHandle usertype and its metatable exactly once.
  void RegisterEntityTypes(sol::state& lua);

  //  Per-scene binding
  // Call on every scene load and reload.
  // Rebinds the Entity table with fresh registry pointers.
  // Does NOT touch the usertype registration.
  inline void BindEntity(sol::state& lua,
                         EntityRegistry& luaRegistry,
                         Registry& ecsRegistry)
  {
    sol::state*      L      = &lua;
    EntityRegistry*  luaReg = &luaRegistry;
    Registry*        ecsReg = &ecsRegistry;

    sol::table entity = lua.create_named_table("Entity");

    entity.set_function("New",
    [L, luaReg, ecsReg](sol::variadic_args args) -> sol::object
    {
      Entity cppEntity = ecsReg->CreateEntity();
      ecsReg->AddComponent<Transform>(cppEntity, Transform{});

      sol::table table = L->create_table();
      table["visible"] = true;
      table["active"]  = true;
      table["name"]    = (args.size() > 0 && args[0].is<std::string>())
                             ? args[0].get<std::string>()
                             : "";
      table["tag"] = "";

      EntityHandle handle{table, cppEntity, ecsReg};
      sol::object  handleObj = sol::make_object(*L, handle);

      luaReg->Register(table, handleObj);

      sol::protected_function onAttach = table["OnAttach"];
      if (onAttach.valid())
        onAttach(handleObj);

      return handleObj;
    });

    entity.set_function("Destroy",
    [luaReg](EntityHandle& handle)
    {
      luaReg->MarkForDestroy(handle.table);
      handle.table["active"] = false;
    });

    entity.set_function("Find",
    [luaReg, L](const std::string& name) -> sol::object
    {
      return luaReg->Find(name, sol::state_view(*L));
    });

    entity.set_function("FindAll",
    [luaReg, L](const std::string& tag) -> sol::table
    {
      return luaReg->FindAll(tag, sol::state_view(*L));
    });

    entity.set_function("FindNear",
    [luaReg, L](float x, float y, float radius) -> sol::table
    {
      return luaReg->FindNear(x, y, radius, sol::state_view(*L));
    });

    entity.set_function("Count",
    [luaReg]() -> int
    {
      return luaReg->Count();
    });
  }

} // namespace Cadmium::Lua

#endif // CADMIUM_LUA_BINDINGS_ENTITY_HPP
