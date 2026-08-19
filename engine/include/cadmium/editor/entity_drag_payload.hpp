#ifndef CADMIUM_EDITOR_ENTITY_DRAG_PAYLOAD_HPP
#define CADMIUM_EDITOR_ENTITY_DRAG_PAYLOAD_HPP

#include <cadmium/ecs/world.hpp>
#include <vector>

namespace Cadmium::Editor
{

// ImGui drag-drop payloads must be a flat, memcpy-able block, so this caps
// how many entities can be dragged in one reparent at once. 64 comfortably
// covers a multi-select drag
struct EntityDragPayload
{
    static constexpr size_t k_MaxEntities = 64;

    Entity entities[k_MaxEntities]{};
    size_t count{0};

    static EntityDragPayload Make(const std::vector<Entity>& selected)
    {
        EntityDragPayload payload;
        payload.count = std::min(selected.size(), k_MaxEntities);
        for (size_t i = 0; i < payload.count; ++i)
            payload.entities[i] = selected[i];
        return payload;
    }
};

inline constexpr const char* k_EntityDragDropId = "CADMIUM_ENTITY_DRAG";

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_ENTITY_DRAG_PAYLOAD_HPP
