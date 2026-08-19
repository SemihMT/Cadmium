#ifndef CADMIUM_EDITOR_WORLD_SNAPSHOT_HPP
#define CADMIUM_EDITOR_WORLD_SNAPSHOT_HPP

#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/component_inspector.hpp>
#include <cadmium/editor/hierarchy_commands.hpp>
#include <any>
#include <optional>
#include <string>
#include <vector>

namespace Cadmium::Editor
{

// Captures/restores the scene around Play/Stop so gameplay mutations don't
// leak into the authored scene.
//
// Note: only components registered with ComponentInspector are preserved.
// Entity handles are not preserved after Restore (all entities are recreated),
// so clear any external Entity references after calling Restore().
class WorldSnapshotService
{
public:
    void Capture(World& world)
    {
        m_Entities.clear();
        m_Captured = true;

        std::vector<Entity> all = world.AllEntities();

        auto indexOf = [&](Entity e) -> std::optional<size_t>
        {
            for (size_t i = 0; i < all.size(); ++i)
                if (all[i] == e)
                    return i;
            return std::nullopt;
        };

        for (Entity e : all)
        {
            EntitySnapshot snap;
            snap.name = GetEntityName(world, e);

            if (auto parent = GetEntityParent(world, e))
                snap.parentIndex = indexOf(*parent);

            for (auto& entry : ComponentInspector::Get().Entries())
                if (entry.has(world, e) && entry.snapshot)
                    snap.components.emplace_back(entry.name, entry.snapshot(world, e));

            m_Entities.push_back(std::move(snap));
        }
    }

    void Restore(World& world)
    {
        if (!m_Captured)
            return; // nothing to restore

        // Rebuild from snapshot: clear current entities, then recreate in order.
        for (Entity e : world.AllEntities())
            world.DestroyEntity(e);
        world.FlushPendingDestroys(); // Flush deferred destroys before recreating.

        std::vector<Entity> recreated;
        recreated.reserve(m_Entities.size());

        for (auto& snap : m_Entities)
        {
            Entity e = CreateBareEntity(world); // always gets a Transform
            if (snap.name.has_value())
                SetEntityName(world, e, snap.name);
            recreated.push_back(e);
        }

        for (size_t i = 0; i < m_Entities.size(); ++i)
        {
            EntitySnapshot& snap = m_Entities[i];
            Entity e = recreated[i];

            for (auto& [componentName, value] : snap.components)
            {
                // CreateBareEntity already adds Transform; skip re-adding.
                if (IsAlwaysPresentComponent(componentName))
                    continue;

                for (auto& entry : ComponentInspector::Get().Entries())
                    if (entry.name == componentName && entry.restore)
                    {
                        // Defensive: avoids AddComponent assert if component already present.
                        if (entry.has(world, e))
                            continue;
                        entry.restore(world, e, value);
                    }
            }

            if (snap.parentIndex.has_value() && *snap.parentIndex < recreated.size())
                world.SetParent(e, recreated[*snap.parentIndex]);
        }
    }

private:
    struct EntitySnapshot
    {
        std::optional<std::string> name;
        std::optional<size_t>      parentIndex; // index into m_Entities/recreated[], not a raw Entity
        std::vector<std::pair<std::string, std::any>> components;
    };

    bool m_Captured{false};
    std::vector<EntitySnapshot> m_Entities;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_WORLD_SNAPSHOT_HPP
