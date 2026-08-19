#ifndef CADMIUM_EDITOR_COMPONENT_COMMANDS_HPP
#define CADMIUM_EDITOR_COMPONENT_COMMANDS_HPP

#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <string>
#include <utility>

namespace Cadmium::Editor
{

// Adds a default-constructed T on Execute(), removes it on Undo(). Backs
// the Inspector's "+ Add Component" menu.
template <typename T>
class AddComponentCommand : public ICommand
{
public:
    AddComponentCommand(World& world, Entity entity, std::string componentName)
        : m_World(world), m_Entity(entity), m_ComponentName(std::move(componentName))
    {}

    void Execute() override { m_World.AddComponent<T>(m_Entity, T{}); }
    void Undo() override    { m_World.RemoveComponent<T>(m_Entity); }
    std::string Description() const override { return "Add " + m_ComponentName; }

private:
    World&      m_World;
    Entity      m_Entity;
    std::string m_ComponentName;
};

// Snapshots the component's current value at construction time (before
// it's actually removed) so Undo() restores what was there rather
// than a blank T{}. Backs the Inspector's per-component remove ("x")
// button on each CollapsingHeader.
template <typename T>
class RemoveComponentCommand : public ICommand
{
public:
    RemoveComponentCommand(World& world, Entity entity, std::string componentName)
        : m_World(world), m_Entity(entity), m_ComponentName(std::move(componentName)),
          m_Snapshot(world.GetComponent<T>(entity))
    {}

    void Execute() override { m_World.RemoveComponent<T>(m_Entity); }
    void Undo() override    { m_World.AddComponent<T>(m_Entity, m_Snapshot); }
    std::string Description() const override { return "Remove " + m_ComponentName; }

private:
    World&      m_World;
    Entity      m_Entity;
    std::string m_ComponentName;
    T           m_Snapshot;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_COMPONENT_COMMANDS_HPP
