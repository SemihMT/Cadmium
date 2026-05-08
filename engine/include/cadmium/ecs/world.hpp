#ifndef CADMIUM_WORLD_HPP
#define CADMIUM_WORLD_HPP

#include <cadmium/ecs/registry.hpp>
#include <cadmium/ecs/system_scheduler.hpp>
#include <cadmium/scripting/lua_type_registry.hpp>
#include <cadmium/ecs/lua_component.hpp>

namespace Cadmium
{
  class World
  {
  public:
    // Lifecycle
    void Start()
    {
      m_Scheduler.Start(*this);
    }

    void Update(float dt)
    {
      m_Scheduler.Update(*this, dt);
    }

    void Stop()
    {
      m_Scheduler.Stop(*this);
    }

    // Entity API - forwarded from Registry
    Entity CreateEntity() { return m_Registry.CreateEntity(); }
    void DestroyEntity(Entity e) { m_Registry.DestroyEntity(e); }
    bool IsValid(Entity e) const { return m_Registry.IsValid(e); }
    size_t EntityCount() const { return m_Registry.EntityCount(); }

    template <typename T>
    void AddComponent(Entity e, T component)
    {
      m_Registry.AddComponent<T>(e, std::move(component));
    }

    template <typename T>
    void RemoveComponent(Entity e) { m_Registry.RemoveComponent<T>(e); }

    template <typename T>
    T &GetComponent(Entity e) { return m_Registry.GetComponent<T>(e); }

    template <typename T>
    const T &GetComponent(Entity e) const { return m_Registry.GetComponent<T>(e); }

    template <typename T>
    bool HasComponent(Entity e) const { return m_Registry.HasComponent<T>(e); }

    template <typename T>
    T *TryGetComponent(Entity e) { return m_Registry.TryGetComponent<T>(e); }

    template <typename T>
    std::vector<std::pair<Entity, T *>> Query()
    {
      return m_Registry.Query<T>();
    }

    template <typename T, typename... Rest>
    std::vector<Entity> QueryEntities()
    {
      return m_Registry.QueryEntities<T, Rest...>();
    }

    // Lua Component API
    // Called by Component.Register binding - registers the schema and
    // assigns it a ComponentID from the global counter
    ComponentID RegisterLuaComponent(LuaComponentSchema schema)
    {
      schema.id = NextComponentID();
      ComponentID id = schema.id;
      m_LuaTypes.Register(std::move(schema));
      return id;
    }

    void AddLuaComponent(Entity e, const std::string &typeName,
                         std::unordered_map<std::string, LuaFieldValue> initValues = {})
    {
      const LuaComponentSchema *schema = m_LuaTypes.Find(typeName);
      if (!schema)
      {
        SDL_Log("[World] Unknown Lua component type: %s", typeName.c_str());
        return;
      }

      LuaComponentData data = LuaComponentData::FromSchema(*schema);
      for (auto &[field, value] : initValues)
      {
        if (!data.values.contains(field))
        {
          SDL_Log("[World] Unknown field '%s' on '%s'", field.c_str(), typeName.c_str());
          continue;
        }
        // Variant index must match - enforced at binding layer
        data.values[field] = std::move(value);
      }

      m_Registry.AddLuaComponent(e, schema->id, std::move(data));
    }

    void RemoveLuaComponent(Entity e, const std::string &typeName)
    {
      const LuaComponentSchema *schema = m_LuaTypes.Find(typeName);
      if (schema)
        m_Registry.RemoveLuaComponent(e, schema->id);
    }

    bool HasLuaComponent(Entity e, const std::string &typeName) const
    {
      const LuaComponentSchema *schema = m_LuaTypes.Find(typeName);
      return schema && m_Registry.HasLuaComponent(e, schema->id);
    }

    LuaComponentData *TryGetLuaComponent(Entity e, const std::string &typeName)
    {
      const LuaComponentSchema *schema = m_LuaTypes.Find(typeName);
      if (!schema)
        return nullptr;
      return m_Registry.TryGetLuaComponent(e, schema->id);
    }

    std::vector<std::pair<Entity, LuaComponentData *>>
    Query(const std::string &typeName)
    {
      const LuaComponentSchema *schema = m_LuaTypes.Find(typeName);
      if (!schema)
        return {};
      return m_Registry.QueryLua(schema->id);
    }

    // Called by EntityRegistry::FlushDestroyed
    void RemoveAllLuaComponents(Entity e)
    {
      std::vector<ComponentID> ids;
      for (const auto &[name, schema] : m_LuaTypes.All())
        ids.push_back(schema.id);
      m_Registry.RemoveAllLuaComponents(e, ids);
    }

    // System API - forwarded from Scheduler
    template <typename T, typename... Args>
    T &RegisterSystem(int order, Args &&...args)
    {
      return m_Scheduler.RegisterSystem<T>(order, std::forward<Args>(args)...);
    }

    template <typename T>
    T &GetSystem() { return m_Scheduler.GetSystem<T>(); }

    template <typename T>
    bool HasSystem() const { return m_Scheduler.HasSystem<T>(); }

    template <typename T>
    void UnregisterSystem() { m_Scheduler.UnregisterSystem<T>(*this); }

    // Direct registry access for systems that need it (scripting layer)
    // C++ systems should use the world's api
    Registry &GetRegistry() { return m_Registry; }
    const Registry &GetRegistry() const { return m_Registry; }

  private:
    Registry m_Registry;
    SystemScheduler m_Scheduler;
    LuaTypeRegistry m_LuaTypes;
  };

} // namespace Cadmium

#endif // CADMIUM_WORLD_HPP
