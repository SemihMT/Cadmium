#ifndef CADMIUM_EDITOR_SELECTION_HPP
#define CADMIUM_EDITOR_SELECTION_HPP

#include <algorithm>
#include <cadmium/ecs/entity.hpp>
#include <cadmium/ecs/world.hpp>
#include <string>
#include <vector>

namespace Cadmium::Editor
{

// Single source of truth for "what's selected" across the editor.
class EditorSelectionContext
{
public:
    // Primary entity selection the single entity the Inspector shows.
    // For a multi-select, this is whichever entity was clicked/toggled
    // most recently (the "focused" one).
    Entity GetEntity() const { return m_Entity; }

    // Full multi-selection (Hierarchy ctrl/shift-click), used for bulk
    // operations (multi-delete, multi-reparent). Includes the primary
    // entity. Empty when nothing is selected.
    const std::vector<Entity>& GetMultiSelection() const { return m_MultiSelected; }

    bool IsSelected(Entity e) const
    {
        return std::find(m_MultiSelected.begin(), m_MultiSelected.end(), e) != m_MultiSelected.end();
    }

    // Replaces the whole selection with just e (a plain click).
    void SelectEntity(Entity e)
    {
        m_Entity = e;
        m_MultiSelected.clear();
        if (e != k_NullEntity)
            m_MultiSelected.push_back(e);
    }

    // Adds/removes e from the selection without disturbing the rest
    // (ctrl-click). e becomes the new primary/focused entity either way,
    // so a follow-up shift-click range-selects from here.
    void ToggleEntity(Entity e)
    {
        auto it = std::find(m_MultiSelected.begin(), m_MultiSelected.end(), e);
        if (it != m_MultiSelected.end())
        {
            m_MultiSelected.erase(it);
            m_Entity = m_MultiSelected.empty() ? k_NullEntity : m_MultiSelected.back();
        }
        else
        {
            m_MultiSelected.push_back(e);
            m_Entity = e;
        }
    }

    // Sets the full multi-selection directly (used for shift-click range
    // select, where the Hierarchy computes the range itself).
    void SetMultiSelection(std::vector<Entity> entities)
    {
        m_MultiSelected = std::move(entities);
        m_Entity = m_MultiSelected.empty() ? k_NullEntity : m_MultiSelected.back();
    }

    void ClearEntity()
    {
        m_Entity = k_NullEntity;
        m_MultiSelected.clear();
    }

    // Drops any selected entities that no longer exist (e.g. after a
    // delete)
    // called after any operation that can destroy entities still
    // referenced by the selection.
    void PruneInvalid(World& world)
    {
        m_MultiSelected.erase(
            std::remove_if(m_MultiSelected.begin(), m_MultiSelected.end(),
                           [&](Entity e) { return !world.IsValid(e); }),
            m_MultiSelected.end());
        if (m_Entity != k_NullEntity && !world.IsValid(m_Entity))
            m_Entity = m_MultiSelected.empty() ? k_NullEntity : m_MultiSelected.back();
    }

    // Asset selection (Asset panel, Script editor)
    const std::string& GetAssetPath() const { return m_AssetPath; }

    void SelectAsset(const std::string& path)
    {
        m_AssetPath = path;
    }

    void ClearAsset()
    {
        m_AssetPath.clear();
    }

private:
    Entity              m_Entity{k_NullEntity};
    std::vector<Entity> m_MultiSelected;
    std::string         m_AssetPath;
};

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_SELECTION_HPP
