#ifndef CADMIUM_TEXT_CACHE
#define CADMIUM_TEXT_CACHE

#include "SDL3/SDL_error.h"
#include "cadmium/core/handles.hpp"
#include "cadmium/core/logger.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cadmium/core/draw_command_queue.hpp>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Cadmium {

class TextCache {
public:
    TextCache() = default;

    void Init(SDL_Renderer* renderer, const std::string& defaultFontPath, float defaultPtSize = 32.0f)
    {
        m_Renderer = renderer;
        m_DefaultFont = LoadFontInternal(defaultFontPath, defaultPtSize);
        if (m_DefaultFont == k_InvalidFont) {
            throw std::runtime_error("Failed to load default font: " + defaultFontPath);
        }
    }

    ~TextCache() { Clear(); }
    TextCache(const TextCache&) = delete;
    TextCache& operator=(const TextCache&) = delete;

    struct Metrics {
        float width{0.f};
        float height{0.f};
    };

    [[nodiscard]] FontHandle LoadFont(const std::string& path, float ptSize = 32.0f) {
        auto it = m_PathToFont.find(path);
        if (it != m_PathToFont.end())
            return it->second;

        FontHandle handle = LoadFontInternal(path, ptSize);
        if (handle == k_InvalidFont) {
            Log::Warn("Text Cache",
                      "Failed to open font ({}), falling back to default. Reason: {}",
                      path,
                      SDL_GetError());
            return m_DefaultFont;
        }
        m_PathToFont[path] = handle;
        return handle;
    }

    [[nodiscard]] FontHandle GetDefaultFont() const noexcept { return m_DefaultFont; }

    // Draw text top-left anchored at (x, y).
    void Draw(FontHandle font,
              const std::string& text,
              float x,
              float y,
              float size,
              const Color& color) {
        const Entry* entry = GetOrCreateTexture(font, text, size, color);
        if (!entry || !entry->texture)
            return;

        const SDL_FRect dst{x, y, entry->width, entry->height};
        SDL_RenderTexture(m_Renderer, entry->texture, nullptr, &dst);
    }

    // Layout-only measurement.
    [[nodiscard]] std::optional<Metrics> Measure(FontHandle font,
                                                  const std::string& text,
                                                  float size) {
        TTF_Font* f = Resolve(font);
        if (!f)
            return std::nullopt;

        TTF_SetFontSize(f, static_cast<int>(size));
        int w = 0, h = 0;
        if (TTF_GetStringSize(f, text.c_str(), 0, &w, &h) != 0)
            return std::nullopt;

        return Metrics{static_cast<float>(w), static_cast<float>(h)};
    }

    void Update() {
        ++m_FrameCounter;
        if (m_FrameCounter % k_SweepInterval != 0)
            return;

        for (auto it = m_Cache.begin(); it != m_Cache.end();) {
            if (m_FrameCounter - it->second.lastUsedFrame > k_EvictAfterFrames) {
                SDL_DestroyTexture(it->second.texture);
                it = m_Cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Clear() {
        for (auto& [key, entry] : m_Cache)
            SDL_DestroyTexture(entry.texture);
        m_Cache.clear();

        for (TTF_Font* font : m_Fonts) {
            if (font)
                TTF_CloseFont(font);
        }
        m_Fonts.clear();
        m_PathToFont.clear();
    }

    [[nodiscard]] size_t CachedEntryCount() const { return m_Cache.size(); }

private:
    static std::array<uint8_t, 4> QuantizeColor(const Color& c) {
        auto toByte = [](float v) {
            return static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f);
        };
        return {toByte(c.r), toByte(c.g), toByte(c.b), toByte(c.a)};
    }

    static SDL_Color ToSDLColor(const Color& c) {
        auto [r, g, b, a] = QuantizeColor(c);
        return {r, g, b, a};
    }

    struct Key {
        std::string text;
        int         size;
        FontHandle  font;
        uint8_t     r, g, b, a;   // quantised colour

        bool operator==(const Key& other) const = default;
    };

    struct KeyHash {
        size_t operator()(const Key& k) const {
            size_t h = std::hash<std::string>{}(k.text);
            auto mix = [&h](auto v) {
                h ^= std::hash<decltype(v)>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
            };
            mix(k.size);
            mix(k.font);
            mix(k.r);
            mix(k.g);
            mix(k.b);
            mix(k.a);
            return h;
        }
    };

    struct Entry {
        SDL_Texture* texture{nullptr};
        float width{0.f};
        float height{0.f};
        uint64_t lastUsedFrame{0};
    };

    FontHandle LoadFontInternal(const std::string& path, float ptSize) {
        TTF_Font* font = TTF_OpenFont(path.c_str(), ptSize);
        if (!font)
            return k_InvalidFont;

        FontHandle handle = static_cast<FontHandle>(m_Fonts.size()) + 1;
        m_Fonts.push_back(font);
        return handle;
    }

    TTF_Font* Resolve(FontHandle handle) const {
        return handle < m_Fonts.size() ? m_Fonts[handle] : nullptr;
    }

    const Entry* GetOrCreateTexture(FontHandle font,
                                     const std::string& text,
                                     float size,
                                     const Color& color) {
        auto col = QuantizeColor(color);
        Key key{text,
                static_cast<int>(size),
                font,
                col[0], col[1], col[2], col[3]};

        auto it = m_Cache.find(key);
        if (it != m_Cache.end()) {
            it->second.lastUsedFrame = m_FrameCounter;
            return &it->second;
        }

        Entry rendered = Render(font, text, size, color);
        if (!rendered.texture)
            return nullptr;

        rendered.lastUsedFrame = m_FrameCounter;
        auto [newIt, _] = m_Cache.emplace(std::move(key), std::move(rendered));
        return &newIt->second;
    }

    Entry Render(FontHandle fontHandle,
                 const std::string& text,
                 float size,
                 const Color& color) {
        TTF_Font* font = Resolve(fontHandle);
        if (!font)
            return {};

        TTF_SetFontSize(font, static_cast<int>(size));
        SDL_Color sdlCol = ToSDLColor(color);

        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, sdlCol);
        if (!surface)
            return {};

        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
        Entry entry{texture,
                    static_cast<float>(surface->w),
                    static_cast<float>(surface->h),
                    0 /* lastUsedFrame set by caller */};
        SDL_DestroySurface(surface);
        return entry;
    }

    FontHandle m_DefaultFont;
    std::vector<TTF_Font*> m_Fonts;                     // index == FontHandle
    std::unordered_map<std::string, FontHandle> m_PathToFont;

    SDL_Renderer* m_Renderer;
    std::unordered_map<Key, Entry, KeyHash> m_Cache;
    uint64_t m_FrameCounter{0};

    static constexpr uint64_t k_EvictAfterFrames = 300;
    static constexpr uint64_t k_SweepInterval    = 60;
};

} // namespace Cadmium

#endif // CADMIUM_TEXT_CACHE
