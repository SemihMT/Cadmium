#ifndef CADMIUM_EDITOR_COMPONENT_EDIT_COMMAND_HPP
#define CADMIUM_EDITOR_COMPONENT_EDIT_COMMAND_HPP

#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <string>
#include <utility>

namespace Cadmium::Editor
{

// Stores before/after snapshots of components
// possible due to components being flat pod structs
// (might need to be reworked if components become more complex)
template <typename T>
class ComponentEditCommand : public ICommand
{
public:
    ComponentEditCommand(World& world, Entity entity, std::string componentName, T before, T after)
        : m_World(world), m_Entity(entity), m_ComponentName(std::move(componentName)),
          m_Before(std::move(before)), m_After(std::move(after))
    {}

    void Execute() override
    {
        if (m_World.IsValid(m_Entity))
            m_World.GetComponent<T>(m_Entity) = m_After;
    }

    void Undo() override
    {
        if (m_World.IsValid(m_Entity))
            m_World.GetComponent<T>(m_Entity) = m_Before;
    }

    std::string Description() const override
    {
        return "Edit " + m_ComponentName;
    }

private:
    World&      m_World;
    Entity      m_Entity;
    std::string m_ComponentName;
    T           m_Before;
    T           m_After;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_COMPONENT_EDIT_COMMAND_HPP
