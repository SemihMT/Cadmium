#ifndef CADMIUM_EDITOR_ASSET_DRAG_PAYLOAD_HPP
#define CADMIUM_EDITOR_ASSET_DRAG_PAYLOAD_HPP

#include <cadmium/assets/asset_types.hpp>
#include <cstring>

namespace Cadmium::Editor
{

// Drag payload shared by AssetPanel, Hierarchy, Inspector etc
struct AssetDragPayload
{
    static constexpr size_t k_MaxPath = 256;

    char      path[k_MaxPath]{};
    AssetType type{AssetType::Unknown};

    static AssetDragPayload Make(const std::string& p, AssetType t)
    {
        AssetDragPayload payload;
        std::snprintf(payload.path, sizeof(payload.path), "%s", p.c_str());
        payload.type = t;
        return payload;
    }
};

// ImGui payload type id
inline constexpr const char* k_AssetDragDropId = "CADMIUM_ASSET_PATH";

} // namespace Cadmium::Editor

#endif // CADMIUM_EDITOR_ASSET_DRAG_PAYLOAD_HPP
