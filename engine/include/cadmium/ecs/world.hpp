#ifndef CADMIUM_WORLD_HPP
#define CADMIUM_WORLD_HPP

#include "cadmium/ecs/entity.hpp"
#include <cadmium/core/logger.hpp>
#include <cadmium/ecs/registry.hpp>
#include <cadmium/ecs/components.hpp>
#include <cadmium/ecs/system_scheduler.hpp>

namespace Cadmium
{
  class Scene;
  class ScriptHost;
  class World
  {
  public:
    // Lifecycle
    void Start();

    void Update(float dt);

    void Stop();

    void SetOwningScene(Scene* scene);
    // Entity API - forwarded from Registry
    Entity CreateEntity();
    void DestroyEntity(Entity e);
    void FlushPendingDestroys();
    bool IsValid(Entity e) const;
    size_t EntityCount() const;

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
    std::vector<Entity> AllEntities();

    glm::mat4 GetWorldMatrix(Entity entity, int depth);

    bool SetParent(Entity child, Entity parent);

    void ClearParent(Entity child) { RemoveComponent<Parent>(child); }

    void OrphanChildrenOf(Entity destroyed);

    // Scripting
    ScriptHost& GetScriptHost();

    // Direct registry access for systems that need it (scripting layer)
    // C++ systems should use the world's api
    Registry& GetRegistry();
    const Registry& GetRegistry() const;

  private:
    std::vector<Entity> m_PendingDestroy;
    Scene* m_OwningScene {nullptr};
    Registry m_Registry;
    SystemScheduler m_Scheduler;
  };

} // namespace Cadmium

#endif // CADMIUM_WORLD_HPP
