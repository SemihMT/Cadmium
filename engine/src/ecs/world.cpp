#include <cadmium/ecs/world.hpp>
#include <cadmium/core/scene.hpp>
#include <cadmium/scripting/script_host.hpp>

namespace Cadmium
{
    void World::Start()
    {
        m_Scheduler.Start(*this);
    }
    void World::Update(float dt)
    {
        m_Scheduler.Update(*this, dt);
    }
    void World::Stop()
    {
        m_Scheduler.Stop(*this);
    }
    void World::SetOwningScene(Scene* scene)
    {
        m_OwningScene = scene;
    }
    Entity World::CreateEntity()
    {
        return m_Registry.CreateEntity();
    }
    void World::DestroyEntity(Entity e)
    {
        OrphanChildrenOf(e);
        m_Scheduler.NotifyEntityDestroyed(*this, e);
        m_Registry.DestroyEntity(e);
    }
    std::vector<Entity> World::AllEntities()
    {
        return m_Registry.AllEntities();
    }
    glm::mat4 World::GetWorldMatrix(Entity entity, int depth)
    {
        glm::mat4 local = GetComponent<Transform>(entity).GetMatrix();

        if (depth >= 64) // cycle guard
            return local;

        if (auto* parent = TryGetComponent<Parent>(entity))
            if (IsValid(parent->entity) && parent->entity != entity)
                return GetWorldMatrix(parent->entity, depth + 1) * local;

        return local;
    }
    bool World::SetParent(Entity child, Entity parent)
    {
        if (child == parent)
            return false;

        Entity walk = parent;
        while (IsValid(walk))
        {
            if (walk == child)
                return false;
            auto* p = TryGetComponent<Parent>(walk);
            if (!p)
                break;
            walk = p->entity;
        }

        AddComponent<Parent>(child, Parent{parent});
        return true;
    }
    void World::OrphanChildrenOf(Entity destroyed)
    {
        for (Entity e : AllEntities())
            if (auto* p = TryGetComponent<Parent>(e))
                if (p->entity == destroyed)
                    RemoveComponent<Parent>(e);
    }

    ScriptHost& World::GetScriptHost()
    {
        return m_OwningScene->GetScriptHost();
    }
    Registry& World::GetRegistry()
    {
        return m_Registry;
    }
    const Registry& World::GetRegistry() const
    {
        return m_Registry;
    }

} // namespace Cadmium
