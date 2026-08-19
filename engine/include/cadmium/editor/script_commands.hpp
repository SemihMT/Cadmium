#ifndef CADMIUM_EDITOR_SCRIPT_COMMANDS_HPP
#define CADMIUM_EDITOR_SCRIPT_COMMANDS_HPP

#include <cadmium/ecs/world.hpp>
#include <cadmium/editor/undo_stack.hpp>
#include <string>
#include <utility>

namespace Cadmium::Editor
{

// Appends a ScriptInstance to the entity's Script component on Execute(),
// removes it again on Undo(). Assumes the Script component already exists
// on the entity by the time this command runs (EditorOverlayLayer::
// AttachScript guarantees that via AddComponentCommand<Script> first).
//
// Undo() just pops the last element rather than tracking an index -
// correct as long as undo/redo stays strictly LIFO,
// since the most recent AddScriptInstanceCommand on this entity
// is always the one undone first.
class AddScriptInstanceCommand : public ICommand
{
public:
    AddScriptInstanceCommand(World& world, Entity entity, ScriptInstance instance)
        : m_World(world), m_Entity(entity), m_Instance(std::move(instance))
    {}

    void Execute() override
    {
        // Copies m_Instance rather than moving it, since Redo()
        // would be left without a script instance otherwise.
        // ScriptInstance's env/functions are sol2 reference-counted
        // handles, so this is a cheap handle copy
        m_World.GetComponent<Script>(m_Entity).instances.push_back(m_Instance);
    }

    void Undo() override
    {
        auto& instances = m_World.GetComponent<Script>(m_Entity).instances;
        if (!instances.empty())
            instances.pop_back();
    }

    std::string Description() const override { return "Attach Script '" + m_Instance.name + "'"; }

private:
    World&         m_World;
    Entity         m_Entity;
    ScriptInstance m_Instance;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SCRIPT_COMMANDS_HPP
