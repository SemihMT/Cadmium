#ifndef CADMIUM_ASSETS_ASSET_MANAGER_HPP
#define CADMIUM_ASSETS_ASSET_MANAGER_HPP

#include "asset_types.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace Cadmium
{

// AssetManager
// Engine-owned. Lives for the duration of the engine session.
// Caches assets by path - loading the same path twice returns the same handle.
// Scripts interact with this through the Assets Lua table.
// The editor asset panel reads from this for display.
class AssetManager
{
public:
    explicit AssetManager() = default;
    ~AssetManager();

    // Not copyable - owns SDL resources
    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    void Init(SDL_Renderer* renderer);

    // Project root:
    // Set by the engine when a project is opened.
    // All relative paths are resolved against this.
    void        SetProjectRoot(const std::string& root);
    const std::string& GetProjectRoot() const { return m_ProjectRoot; }

    // Resolve a relative path to an absolute path using the project root.
    std::string ResolvePath(const std::string& relativePath) const;

    // Loading:
    // All paths are relative to the project root.
    // Returns k_InvalidHandle on failure - never throws.

    TextureHandle LoadTexture(const std::string& path);
    FontHandle    LoadFont(const std::string& path, int size);
    // SoundHandle LoadSound(const std::string& path); // phase 4

    // Retrieval:
    // Used internally by ScriptRenderLayer and other engine systems.
    // Returns nullptr for invalid handles.

    SDL_Texture* GetTexture(TextureHandle handle) const;
    TTF_Font*    GetFont(FontHandle handle) const;

    // Unloading
    void UnloadTexture(TextureHandle handle);
    void UnloadFont(FontHandle handle);
    void UnloadAll();

    // File discovery:
    // Scans the project root and builds the asset entry list.
    // Called when project root changes and on explicit refresh.
    void ScanProjectFiles();

    // Returns all discovered asset entries (loaded and unloaded).
    // Used by the asset panel for display.
    const std::vector<AssetEntry>& GetAllEntries() const { return m_Entries; }

    // Returns only entries of a specific type.
    std::vector<const AssetEntry*> GetEntriesOfType(AssetType type) const;

    // Force a rescan - call after creating or deleting project files.
    void Refresh() { ScanProjectFiles(); }

    // Texture preview:
    // Returns a texture handle suitable for ImGui::Image().
    // Loads the texture if not already loaded.
    // Used by the asset panel for thumbnail display.
    TextureHandle GetPreviewHandle(const std::string& path);

private:
    SDL_Renderer* m_Renderer = nullptr;
    std::string   m_ProjectRoot;

    // Texture storage:
    std::unordered_map<std::string, TextureHandle> m_TexturePathIndex;
    std::unordered_map<TextureHandle, SDL_Texture*> m_Textures;

    // Font storage:
    // Fonts are keyed by path+size - same file at different sizes = different handle
    std::unordered_map<std::string, FontHandle>    m_FontPathIndex;
    std::unordered_map<FontHandle, TTF_Font*>      m_Fonts;

    // Asset entries (for panel display)
    std::vector<AssetEntry> m_Entries;

    // Handle generation
    uint32_t m_NextHandle = 1; // 0 reserved for k_InvalidHandle

    uint32_t NextHandle() { return m_NextHandle++; }

    // Build font cache key - path + size combined
    static std::string FontKey(const std::string& path, int size)
    {
        return path + ":" + std::to_string(size);
    }

    // Update or insert an AssetEntry for the given path
    void UpdateEntry(const std::string& relativePath,
                     AssetType type,
                     uint32_t handle,
                     int w = 0, int h = 0, int fontSize = 0);
};

} // namespace Cadmium

#endif // CADMIUM_ASSETS_ASSET_MANAGER_HPP
