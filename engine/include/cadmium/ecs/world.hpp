#ifndef CADMIUM_WORLD_HPP
#define CADMIUM_WORLD_HPP

#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/registry.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/ecs/system_scheduler.hpp>

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
    void DestroyEntity(Entity e)
    {
        OrphanChildrenOf(e);
        m_Scheduler.NotifyEntityDestroyed(*this,e);
        m_Registry.DestroyEntity(e);
    }
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

    // ECS Scenegraph methods
    std::vector<Entity> AllEntities() { return m_Registry.AllEntities(); }

    glm::mat4 GetWorldMatrix(Entity entity, int depth)
    {
        glm::mat4 local = GetComponent<Transform>(entity).GetMatrix();

        if (depth >= 64) // cycle guard
            return local;

        if (auto* parent = TryGetComponent<Parent>(entity))
            if (IsValid(parent->entity) && parent->entity != entity)
                return GetWorldMatrix(parent->entity, depth + 1) * local;

        return local;
    }

    bool SetParent(Entity child, Entity parent)
    {
        if (child == parent) return false;

        Entity walk = parent;
        while (IsValid(walk))
        {
            if (walk == child) return false;
            auto* p = TryGetComponent<Parent>(walk);
            if (!p) break;
            walk = p->entity;
        }

        AddComponent<Parent>(child, Parent{parent});
        return true;
    }

    void ClearParent(Entity child) { RemoveComponent<Parent>(child); }

    void OrphanChildrenOf(Entity destroyed)
    {
        for (Entity e : AllEntities())
            if (auto* p = TryGetComponent<Parent>(e))
                if (p->entity == destroyed)
                    RemoveComponent<Parent>(e);
    }

    // Direct registry access for systems that need it (scripting layer)
    // C++ systems should use the world's api
    Registry &GetRegistry() { return m_Registry; }
    const Registry &GetRegistry() const { return m_Registry; }

  private:
    Registry m_Registry;
    SystemScheduler m_Scheduler;
  };

} // namespace Cadmium

#endif // CADMIUM_WORLD_HPP
