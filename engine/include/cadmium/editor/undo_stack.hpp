#ifndef CADMIUM_EDITOR_UNDO_STACK_HPP
#define CADMIUM_EDITOR_UNDO_STACK_HPP

#include <deque>
#include <functional>
#include <memory>
#include <string>

namespace Cadmium::Editor
{

// Base type for any reversible editor action.
class ICommand
{
public:
    virtual ~ICommand() = default;

    // Perform (or re-perform, for Redo) the action.
    virtual void Execute() = 0;

    // Reverse the action.
    virtual void Undo() = 0;

    // Shown in the Edit menu ("Undo <Description>"). Optional.
    virtual std::string Description() const { return ""; }
};

// Single editor-wide undo/redo history.
class UndoStack
{
public:
    static UndoStack& Get()
    {
        static UndoStack instance;
        return instance;
    }

    // Use when the caller has already performed the action (e.g. an ImGui
    // widget already mutated the value directly while being dragged)
    // this just records it so it becomes undoable/redoable.
    void Push(std::unique_ptr<ICommand> cmd)
    {
        m_Undo.push_back(std::move(cmd));
        if (m_Undo.size() > k_MaxDepth)
            m_Undo.pop_front();
        m_Redo.clear(); // a fresh action invalidates any redo history
        NotifyChanged();
    }

    // Use when the stack itself should perform the action (e.g. a "Delete
    // Entity" menu item, as opposed to a value already changed by a
    // live-dragged widget).
    void ExecuteAndPush(std::unique_ptr<ICommand> cmd)
    {
        cmd->Execute();
        Push(std::move(cmd));
    }

    bool CanUndo() const { return !m_Undo.empty(); }
    bool CanRedo() const { return !m_Redo.empty(); }

    void Undo()
    {
        if (m_Undo.empty()) return;
        auto cmd = std::move(m_Undo.back());
        m_Undo.pop_back();
        cmd->Undo();
        m_Redo.push_back(std::move(cmd));
        NotifyChanged();
    }

    void Redo()
    {
        if (m_Redo.empty()) return;
        auto cmd = std::move(m_Redo.back());
        m_Redo.pop_back();
        cmd->Execute();
        m_Undo.push_back(std::move(cmd));
        NotifyChanged();
    }

    // For the Edit menu label ("Undo Edit Sprite"). Empty string if nothing
    // to undo/redo.
    std::string PeekUndoLabel() const
    {
        return m_Undo.empty() ? std::string{} : m_Undo.back()->Description();
    }

    std::string PeekRedoLabel() const
    {
        return m_Redo.empty() ? std::string{} : m_Redo.back()->Description();
    }

    // OnChanged callback called after any Push/ExecuteAndPush/Undo/Redo
    // the editor uses this to drive the scene dirty indicator without every command needing to
    // know about dirty-tracking itself.
    void SetOnChanged(std::function<void()> cb) { m_OnChanged = std::move(cb); }

    // Call when loading/resetting a scene so stale commands belonging to a different scene don't persist
    void Clear()
    {
        m_Undo.clear();
        m_Redo.clear();
    }

private:
    UndoStack() = default;

    void NotifyChanged() { if (m_OnChanged) m_OnChanged(); }

    static constexpr size_t k_MaxDepth = 200;

    std::deque<std::unique_ptr<ICommand>> m_Undo;
    std::deque<std::unique_ptr<ICommand>> m_Redo;
    std::function<void()>                 m_OnChanged;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_UNDO_STACK_HPP
