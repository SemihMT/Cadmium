#ifndef CADMIUM_SCRIPTING_LUA_BINDINGS_ASSETS_HPP
#define CADMIUM_SCRIPTING_LUA_BINDINGS_ASSETS_HPP
#include <cadmium/assets/asset_manager.hpp>
#include <sol/sol.hpp>

namespace Cadmium::Lua
{

// Binds the Assets table into Lua.
// Scripts use:
//   local tex = Assets.LoadTexture("sprites/player.png")
//   local fnt = Assets.LoadFont("fonts/mono.ttf", 24)
//   Draw.Sprite(tex, x, y, w, h)
//
// Handles are opaque integers to Lua - 0 is always invalid.
inline void BindAssets(sol::state& lua, AssetManager& assets)
{
    AssetManager* mgr = &assets;

    sol::table tbl = lua.create_named_table("Assets");

    // Assets.LoadTexture(path) → handle (integer)
    // path is relative to project root
    tbl.set_function("LoadTexture",
        [mgr](const std::string& path) -> uint32_t
        {
            return mgr->LoadTexture(path);
        });

    // Assets.LoadFont(path, size) → handle (integer)
    tbl.set_function("LoadFont",
        [mgr](const std::string& path, int size) -> uint32_t
        {
            return mgr->LoadFont(path, size);
        });

    // Assets.IsValid(handle) → bool
    // Scripts can check if a load succeeded
    tbl.set_function("IsValid",
        [](uint32_t handle) -> bool
        {
            return handle != k_InvalidHandle;
        });

    // Assets.Unload(handle)
    // Explicit unload - normally not needed, engine unloads on shutdown
    tbl.set_function("Unload",
        [mgr](uint32_t handle)
        {
            // Try texture first, then font
            // In a typed system you'd know which - for now try both
            mgr->UnloadTexture(handle);
            mgr->UnloadFont(handle);
        });
}

} // namespace Cadmium::Lua
#endif // CADMIUM_SCRIPTING_LUA_BINDINGS_ASSETS_HPP
