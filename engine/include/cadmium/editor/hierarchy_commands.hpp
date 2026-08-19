#ifndef CADMIUM_EDITOR_HIERARCHY_COMMANDS_HPP
#define CADMIUM_EDITOR_HIERARCHY_COMMANDS_HPP

#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cadmium::Editor
{

// Creates a bare entity with a default Transform
inline Entity CreateBareEntity(World& world)
{
    Entity e = world.CreateEntity();
    if (!world.HasComponent<Transform>(e))
        world.AddComponent<Transform>(e, Transform{});
    return e;
}

// Creates a new entity (with its default Transform
// with a Tag{name} and, if parent is set, reparents it via World::SetParent.
inline Entity CreateNamedEntity(World& world, const std::string& name, std::optional<Entity> parent)
{
    Entity e = CreateBareEntity(world);
    world.AddComponent<Tag>(e, Tag{name});
    if (parent.has_value())
        world.SetParent(e, *parent);
    return e;
}

// Sets, changes, or clears (nullopt) an entity's display name.
inline void SetEntityName(World& world, Entity e, const std::optional<std::string>& name)
{
    if (!name.has_value())
    {
        world.RemoveComponent<Tag>(e);
        return;
    }
    if (auto* tag = world.TryGetComponent<Tag>(e))
        tag->name = *name;
    else
        world.AddComponent<Tag>(e, Tag{*name});
}

// Reads the current name, if any (for building undo snapshots and display).
inline std::optional<std::string> GetEntityName(World& world, Entity e)
{
    if (auto* tag = world.TryGetComponent<Tag>(e))
        return tag->name;
    return std::nullopt;
}

// Reads the current parent, if any.
inline std::optional<Entity> GetEntityParent(World& world, Entity e)
{
    if (auto* p = world.TryGetComponent<Parent>(e))
        return p->entity;
    return std::nullopt;
}

// Reparents e under newParent via World::SetParent, or unparents to root
// via World::ClearParent if newParent is nullopt.
inline void ReparentEntity(World& world, Entity e, std::optional<Entity> newParent)
{
    if (!newParent.has_value())
        world.ClearParent(e);
    else
        world.SetParent(e, *newParent);
}

// Direct children of e, in AllEntities() order.
inline std::vector<Entity> GetChildren(World& world, Entity e)
{
    std::vector<Entity> children;
    for (Entity candidate : world.AllEntities())
        if (auto* p = world.TryGetComponent<Parent>(candidate))
            if (p->entity == e)
                children.push_back(candidate);
    return children;
}

// True if `candidate` is e itself or anywhere in e's subtree
// used to block a drag-reparent that would create a cycle (dropping a node onto
// its own descendant)
inline bool IsSelfOrDescendant(World& world, Entity e, Entity candidate)
{
    if (e == candidate)
        return true;
    for (Entity child : GetChildren(world, e))
        if (IsSelfOrDescendant(world, child, candidate))
            return true;
    return false;
}

// Whole subtree rooted at e (e included), depth-first.
inline void CollectSubtree(World& world, Entity e, std::vector<Entity>& out)
{
    out.push_back(e);
    for (Entity child : GetChildren(world, e))
        CollectSubtree(world, child, out);
}

// Destroys e and its entire subtree. Not undoable
// see the comment on DeleteSelection in hierarchy_panel.hpp for why.
inline void DestroySubtree(World& world, Entity e)
{
    std::vector<Entity> toDestroy;
    CollectSubtree(world, e, toDestroy);
    for (Entity victim : toDestroy)
        world.DestroyEntity(victim);
}

// ============================================================================
// Commands
// ============================================================================

// Creates a named entity on Execute(), destroys it on Undo(). Redo() after
// an Undo() creates a fresh entity rather than resurrecting the original
// handle (ECS entity indices get recycled once destroyed)
// functionally equivalent
class CreateEntityCommand : public ICommand
{
public:
    CreateEntityCommand(World& world, std::string name, std::optional<Entity> parent)
        : m_World(world), m_Name(std::move(name)), m_Parent(parent)
    {}

    void Execute() override
    {
        m_Entity = CreateNamedEntity(m_World, m_Name, m_Parent);
    }

    void Undo() override
    {
        if (m_Entity.has_value() && m_World.IsValid(*m_Entity))
            DestroySubtree(m_World, *m_Entity);
        m_Entity.reset();
    }

    std::string Description() const override { return "Create Entity"; }

    // Valid only right after Execute(), before any Undo(). Lets the caller
    // select the newly created entity immediately.
    std::optional<Entity> GetCreatedEntity() const { return m_Entity; }

private:
    World&                 m_World;
    std::string             m_Name;
    std::optional<Entity>  m_Parent;
    std::optional<Entity>  m_Entity;
};

class RenameEntityCommand : public ICommand
{
public:
    RenameEntityCommand(World& world, Entity entity,
                        std::optional<std::string> before, std::optional<std::string> after)
        : m_World(world), m_Entity(entity), m_Before(std::move(before)), m_After(std::move(after))
    {}

    void Execute() override { SetEntityName(m_World, m_Entity, m_After); }
    void Undo() override    { SetEntityName(m_World, m_Entity, m_Before); }

    std::string Description() const override { return "Rename Entity"; }

private:
    World&                      m_World;
    Entity                      m_Entity;
    std::optional<std::string> m_Before;
    std::optional<std::string> m_After;
};

class ReparentEntityCommand : public ICommand
{
public:
    ReparentEntityCommand(World& world, Entity entity,
                          std::optional<Entity> before, std::optional<Entity> after)
        : m_World(world), m_Entity(entity), m_Before(before), m_After(after)
    {}

    void Execute() override { ReparentEntity(m_World, m_Entity, m_After); }
    void Undo() override    { ReparentEntity(m_World, m_Entity, m_Before); }

    std::string Description() const override { return "Reparent Entity"; }

private:
    World&                 m_World;
    Entity                 m_Entity;
    std::optional<Entity>  m_Before;
    std::optional<Entity>  m_After;
};

// Bundles several commands (e.g. reparenting a multi-selection in one drag)
// into a single undo step.
class CompositeCommand : public ICommand
{
public:
    CompositeCommand(std::string description, std::vector<std::unique_ptr<ICommand>> commands)
        : m_Description(std::move(description)), m_Commands(std::move(commands))
    {}

    void Execute() override
    {
        for (auto& cmd : m_Commands)
            cmd->Execute();
    }

    void Undo() override
    {
        for (auto it = m_Commands.rbegin(); it != m_Commands.rend(); ++it)
            (*it)->Undo();
    }

    std::string Description() const override { return m_Description; }

private:
    std::string                            m_Description;
    std::vector<std::unique_ptr<ICommand>> m_Commands;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_HIERARCHY_COMMANDS_HPP
