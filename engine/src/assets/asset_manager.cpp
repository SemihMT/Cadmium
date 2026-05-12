#include <cadmium/assets/asset_manager.hpp>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace Cadmium
{
AssetManager::~AssetManager()
{
    UnloadAll();
}

void AssetManager::Init(SDL_Renderer *renderer)
{
  m_Renderer = renderer;
}

//  Project root

void AssetManager::SetProjectRoot(const std::string &root)
{
    m_ProjectRoot = root;
    ScanProjectFiles();
}

std::string AssetManager::ResolvePath(const std::string& relativePath) const
{
    if (m_ProjectRoot.empty())
        return relativePath;

    fs::path resolved = fs::path(m_ProjectRoot) / relativePath;
    return resolved.string();
}

//  Loading

TextureHandle AssetManager::LoadTexture(const std::string& path)
{
    // Return cached handle if already loaded
    auto it = m_TexturePathIndex.find(path);
    if (it != m_TexturePathIndex.end())
        return it->second;

    std::string fullPath = ResolvePath(path);
    SDL_Texture* texture = IMG_LoadTexture(m_Renderer, fullPath.c_str());

    if (!texture)
    {
        SDL_Log("[AssetManager] Failed to load texture '%s': %s",
                path.c_str(), SDL_GetError());
        return k_InvalidHandle;
    }

    TextureHandle handle = NextHandle();
    m_TexturePathIndex[path]    = handle;
    m_Textures[handle]          = texture;

    // Get dimensions for the entry
    float w = 0, h = 0;
    SDL_GetTextureSize(texture, &w, &h);

    UpdateEntry(path, AssetType::Texture, handle, (int)w, (int)h);

    SDL_Log("[AssetManager] Loaded texture '%s' (%dx%d) → handle %u",
            path.c_str(), (int)w, (int)h, handle);

    return handle;
}

FontHandle AssetManager::LoadFont(const std::string& path, int size)
{
    std::string key = FontKey(path, size);

    auto it = m_FontPathIndex.find(key);
    if (it != m_FontPathIndex.end())
        return it->second;

    std::string fullPath = ResolvePath(path);
    TTF_Font* font = TTF_OpenFont(fullPath.c_str(), size);

    if (!font)
    {
        SDL_Log("[AssetManager] Failed to load font '%s' at size %d: %s",
                path.c_str(), size, SDL_GetError());
        return k_InvalidHandle;
    }

    FontHandle handle = NextHandle();
    m_FontPathIndex[key] = handle;
    m_Fonts[handle]      = font;

    UpdateEntry(path, AssetType::Font, handle, 0, 0, size);

    SDL_Log("[AssetManager] Loaded font '%s' size %d → handle %u",
            path.c_str(), size, handle);

    return handle;
}

ScriptHandle AssetManager::LoadScript(const std::string &path)
{
  auto it = m_ScriptPathIndex.find(path);
    if (it != m_ScriptPathIndex.end())
        return it->second;

    std::string fullPath = ResolvePath(path);

    // Read source text from disk
    std::ifstream file(fullPath);
    if (!file.is_open())
    {
        SDL_Log("[AssetManager] Failed to open script '%s'", path.c_str());
        return k_InvalidHandle;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string source = oss.str();

    ScriptHandle handle = NextHandle();
    m_ScriptPathIndex[path] = handle;
    m_Scripts[handle]       = source;

    UpdateEntry(path, AssetType::Script, handle);

    SDL_Log("[AssetManager] Loaded script '%s' → handle %u", path.c_str(), handle);

    return handle;
}
//  Retrieval

SDL_Texture* AssetManager::GetTexture(TextureHandle handle) const
{
    if (handle == k_InvalidHandle) return nullptr;
    auto it = m_Textures.find(handle);
    return it != m_Textures.end() ? it->second : nullptr;
}

TTF_Font* AssetManager::GetFont(FontHandle handle) const
{
    if (handle == k_InvalidHandle) return nullptr;
    auto it = m_Fonts.find(handle);
    return it != m_Fonts.end() ? it->second : nullptr;
}

const std::string *AssetManager::GetScriptSource(ScriptHandle handle) const
{
    if (handle == k_InvalidHandle)
        return nullptr;
    auto it = m_Scripts.find(handle);
    return it != m_Scripts.end() ? &it->second : nullptr;
}

//  Unloading

void AssetManager::UnloadTexture(TextureHandle handle)
{
    auto it = m_Textures.find(handle);
    if (it == m_Textures.end()) return;

    SDL_DestroyTexture(it->second);
    m_Textures.erase(it);

    // Remove from path index
    for (auto pit = m_TexturePathIndex.begin();
         pit != m_TexturePathIndex.end(); ++pit)
    {
        if (pit->second == handle)
        {
            m_TexturePathIndex.erase(pit);
            break;
        }
    }

    // Mark entry as unloaded
    for (auto& entry : m_Entries)
    {
        if (entry.handle == handle)
        {
            entry.loaded = false;
            break;
        }
    }
}

void AssetManager::UnloadFont(FontHandle handle)
{
    auto it = m_Fonts.find(handle);
    if (it == m_Fonts.end()) return;

    TTF_CloseFont(it->second);
    m_Fonts.erase(it);

    for (auto pit = m_FontPathIndex.begin();
         pit != m_FontPathIndex.end(); ++pit)
    {
        if (pit->second == handle)
        {
            m_FontPathIndex.erase(pit);
            break;
        }
    }

    for (auto& entry : m_Entries)
    {
        if (entry.handle == handle)
        {
            entry.loaded = false;
            break;
        }
    }
}

void AssetManager::UnloadScript(ScriptHandle handle)
{
    auto it = m_Scripts.find(handle);
    if (it == m_Scripts.end())
        return;

    m_Scripts.erase(it);

    for (auto pit = m_ScriptPathIndex.begin();
         pit != m_ScriptPathIndex.end(); ++pit)
    {
        if (pit->second == handle)
        {
            m_ScriptPathIndex.erase(pit);
            break;
        }
    }

    for (auto &entry : m_Entries)
    {
        if (entry.handle == handle)
        {
            entry.loaded = false;
            break;
        }
    }
}

void AssetManager::UnloadAll()
{
    for (auto& [handle, texture] : m_Textures)
        SDL_DestroyTexture(texture);
    m_Textures.clear();
    m_TexturePathIndex.clear();

    for (auto& [handle, font] : m_Fonts)
        TTF_CloseFont(font);
    m_Fonts.clear();
    m_FontPathIndex.clear();

    m_Scripts.clear();
    m_ScriptPathIndex.clear();

    // Mark all entries as unloaded but keep them for display
    for (auto& entry : m_Entries)
        entry.loaded = false;
}

//  File discovery

void AssetManager::ScanProjectFiles()
{
    m_Entries.clear();

    if (m_ProjectRoot.empty()) return;

    fs::path root(m_ProjectRoot);
    if (!fs::exists(root) || !fs::is_directory(root))
    {
        SDL_Log("[AssetManager] Project root does not exist: %s",
                m_ProjectRoot.c_str());
        return;
    }

    // Walk all files recursively
    std::error_code ec;
    for (const auto& dirEntry : fs::recursive_directory_iterator(root, ec))
    {
        if (!dirEntry.is_regular_file()) continue;

        fs::path filePath  = dirEntry.path();
        std::string ext    = filePath.extension().string();

        // Lowercase the extension for comparison
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        AssetType type = AssetTypeFromExtension(ext);
        if (type == AssetType::Unknown) continue;

        // Store as relative path from project root
        std::string relativePath = fs::relative(filePath, root, ec).string();

        // Normalise separators to forward slash
        std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

        AssetEntry entry;
        entry.handle   = k_InvalidHandle; // not loaded yet
        entry.type     = type;
        entry.path     = relativePath;
        entry.filename = filePath.filename().string();
        entry.loaded   = false;

        m_Entries.push_back(std::move(entry));
    }

    // Sort: by type first, then alphabetically by path
    std::sort(m_Entries.begin(), m_Entries.end(),
        [](const AssetEntry& a, const AssetEntry& b)
        {
            if (a.type != b.type)
                return (int)a.type < (int)b.type;
            return a.path < b.path;
        });

    SDL_Log("[AssetManager] Scanned project: found %zu asset files",
            m_Entries.size());
}

std::vector<const AssetEntry*>
AssetManager::GetEntriesOfType(AssetType type) const
{
    std::vector<const AssetEntry*> result;
    for (const auto& entry : m_Entries)
        if (entry.type == type)
            result.push_back(&entry);
    return result;
}

TextureHandle AssetManager::GetPreviewHandle(const std::string& path)
{
    // If already loaded return existing handle
    auto it = m_TexturePathIndex.find(path);
    if (it != m_TexturePathIndex.end())
        return it->second;

    // Load on demand for preview
    return LoadTexture(path);
}

//  Internal

void AssetManager::UpdateEntry(const std::string& relativePath,
                                AssetType type,
                                uint32_t handle,
                                int w, int h, int fontSize)
{
    // Update existing entry if found
    for (auto& entry : m_Entries)
    {
        if (entry.path == relativePath)
        {
            entry.handle   = handle;
            entry.loaded   = true;
            entry.width    = w;
            entry.height   = h;
            entry.fontSize = fontSize;
            return;
        }
    }

    // Not in scan results - add it (e.g. loaded from absolute path)
    fs::path p(relativePath);
    AssetEntry entry;
    entry.handle   = handle;
    entry.type     = type;
    entry.path     = relativePath;
    entry.filename = p.filename().string();
    entry.loaded   = true;
    entry.width    = w;
    entry.height   = h;
    entry.fontSize = fontSize;
    m_Entries.push_back(std::move(entry));
}

} // namespace Cadmium
