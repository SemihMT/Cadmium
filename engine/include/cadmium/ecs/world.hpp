#ifndef CADMIUM_WORLD_HPP
#define CADMIUM_WORLD_HPP

#include <cadmium/ecs/registry.hpp>
#include <cadmium/ecs/system_scheduler.hpp>

namespace Cadmium
{
  class World
  {
  public:
    // -----------------------------------------------------------------------
    // Lifecycle - called by Scene
    // -----------------------------------------------------------------------

    void Start()
    {
      m_Scheduler.Start(m_Registry);
    }

    void Update(float dt)
    {
      m_Scheduler.Update(m_Registry, dt);
    }

    void Stop()
    {
      m_Scheduler.Stop(m_Registry);
    }

    // -----------------------------------------------------------------------
    // Entity API - forwarded from Registry
    // -----------------------------------------------------------------------

    Entity CreateEntity()                    { return m_Registry.CreateEntity(); }
    void   DestroyEntity(Entity e)           { m_Registry.DestroyEntity(e); }
    bool   IsValid(Entity e) const           { return m_Registry.IsValid(e); }
    size_t EntityCount() const               { return m_Registry.EntityCount(); }

    template<typename T>
    void AddComponent(Entity e, T component)
    {
      m_Registry.AddComponent<T>(e, std::move(component));
    }

    template<typename T>
    void RemoveComponent(Entity e)           { m_Registry.RemoveComponent<T>(e); }

    template<typename T>
    T& GetComponent(Entity e)               { return m_Registry.GetComponent<T>(e); }

    template<typename T>
    const T& GetComponent(Entity e) const   { return m_Registry.GetComponent<T>(e); }

    template<typename T>
    bool HasComponent(Entity e) const       { return m_Registry.HasComponent<T>(e); }

    template<typename T>
    T* TryGetComponent(Entity e)            { return m_Registry.TryGetComponent<T>(e); }

    template<typename T>
    std::vector<std::pair<Entity, T*>> Query()
    {
      return m_Registry.Query<T>();
    }

    template<typename T, typename... Rest>
    std::vector<Entity> QueryEntities()
    {
      return m_Registry.QueryEntities<T, Rest...>();
    }

    // -----------------------------------------------------------------------
    // System API - forwarded from Scheduler
    // -----------------------------------------------------------------------

    template<typename T, typename... Args>
    T& RegisterSystem(int order, Args&&... args)
    {
      return m_Scheduler.RegisterSystem<T>(order, std::forward<Args>(args)...);
    }

    template<typename T>
    T& GetSystem()                          { return m_Scheduler.GetSystem<T>(); }

    template<typename T>
    bool HasSystem() const                  { return m_Scheduler.HasSystem<T>(); }

    template<typename T>
    void UnregisterSystem()                 { m_Scheduler.UnregisterSystem<T>(m_Registry); }

    // Direct registry access for systems that need it
    Registry& GetRegistry()                 { return m_Registry; }
    const Registry& GetRegistry() const     { return m_Registry; }

  private:
    Registry        m_Registry;
    SystemScheduler m_Scheduler;
  };

} // namespace Cadmium

#endif // CADMIUM_WORLD_HPP
