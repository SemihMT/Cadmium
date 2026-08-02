#ifndef CADMIUM_SDL_RENDERER
#define CADMIUM_SDL_RENDERER
#include "cadmium/core/handles.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cadmium/render/renderer.hpp>
#include <cadmium/render/text_cache.hpp>
#include <cmath>
#include <numbers>
#include <unordered_map>
#include <vector>


namespace Cadmium
{
    // SDL_Renderer-backed implementation of IRenderer. Owns GPU texture
    // storage (resolves GPUTextureHandle -> SDL_Texture*) and camera state;
    // delegates all text work to a shared TextCache rather than touching
    // TTF directly.
    class SDLRenderer : public IRenderer
    {
      public:
        SDLRenderer(SDL_Renderer* renderer, TextCache& textCache)
            : m_Renderer(renderer), m_TextCache(textCache)
        {
        }

        ~SDLRenderer() override
        {
            for (auto& [handle, stored] : m_Textures)
                SDL_DestroyTexture(stored.texture);
        }

        SDLRenderer(const SDLRenderer&) = delete;
        SDLRenderer& operator=(const SDLRenderer&) = delete;

        // Escape hatch for code that hasn't migrated off raw SDL calls yet
        // (every sandbox layer, today). Remove call sites one at a time as
        // they migrate to IRenderer; once nothing calls this, delete it.
        SDL_Renderer* GetNativeHandle() const { return m_Renderer; }

        void BeginFrame() override
        {
            // Cached once per frame rather than re-queried per draw call.
            // SDL_GetCurrentRenderOutputSize (not the plain
            // SDL_GetRenderOutputSize) so this stays correct if a render
            // target is active - e.g. the editor's viewport texture -
            // rather than always reporting the window's own size.
            int w = 0, h = 0;
            SDL_GetCurrentRenderOutputSize(m_Renderer, &w, &h);
            m_ScreenWidth = static_cast<float>(w);
            m_ScreenHeight = static_cast<float>(h);
        }

        void EndFrame() override { m_TextCache.Update(); }

        void DrawLine(const DrawCmd::Line& l) override
        {
            SetColor(l.color);
            float x1, y1, x2, y2;
            ToScreen(l.x1, l.y1, x1, y1);
            ToScreen(l.x2, l.y2, x2, y2);
            SDL_RenderLine(m_Renderer, x1, y1, x2, y2);
        }

        void DrawRect(const DrawCmd::Rect& r) override
        {
            SetColor(r.color);
            float x, y;
            ToScreen(r.x, r.y, x, y);
            SDL_FRect rect{x - r.w * 0.5f * m_CamZoom,
                           y - r.h * 0.5f * m_CamZoom,
                           r.w * m_CamZoom,
                           r.h * m_CamZoom};
            if (r.filled)
                SDL_RenderFillRect(m_Renderer, &rect);
            else
                SDL_RenderRect(m_Renderer, &rect);
        }

        void DrawCircle(const DrawCmd::Circle& c) override
        {
            SetColor(c.color);
            float cx, cy;
            ToScreen(c.x, c.y, cx, cy);
            float radius = c.radius * m_CamZoom;
            int segments =
                c.segments > 0 ? c.segments : std::max(12, static_cast<int>(radius * 0.5f));

            if (c.filled)
            {
                // Triangle fan from the center - fine for a debug/gameplay
                // circle, not a general geometry path.
                std::vector<SDL_Vertex> verts;
                verts.reserve(segments + 2);
                SDL_FColor col{c.color.r, c.color.g, c.color.b, c.color.a};
                verts.push_back(SDL_Vertex{{cx, cy}, col, {0, 0}});
                for (int i = 0; i <= segments; ++i)
                {
                    float t = (static_cast<float>(i) / segments) * 2.0f * std::numbers::pi_v<float>;
                    verts.push_back(SDL_Vertex{
                        {cx + std::cos(t) * radius, cy + std::sin(t) * radius}, col, {0, 0}});
                }
                std::vector<int> indices;
                indices.reserve(segments * 3);
                for (int i = 1; i < segments + 1; ++i)
                {
                    indices.push_back(0);
                    indices.push_back(i);
                    indices.push_back(i + 1);
                }
                SDL_RenderGeometry(m_Renderer,
                                   nullptr,
                                   verts.data(),
                                   static_cast<int>(verts.size()),
                                   indices.data(),
                                   static_cast<int>(indices.size()));
            }
            else
            {
                float prevX = cx + radius, prevY = cy;
                for (int i = 1; i <= segments; ++i)
                {
                    float t = (static_cast<float>(i) / segments) * 2.0f * std::numbers::pi_v<float>;
                    float nx = cx + std::cos(t) * radius;
                    float ny = cy + std::sin(t) * radius;
                    SDL_RenderLine(m_Renderer, prevX, prevY, nx, ny);
                    prevX = nx;
                    prevY = ny;
                }
            }
        }

        void DrawPolygon(const DrawCmd::Polygon& p) override
        {
            if (p.points.size() < 2)
                return;
            SetColor(p.color);

            std::vector<SDL_FPoint> screen;
            screen.reserve(p.points.size());
            for (auto& pt : p.points)
            {
                float sx, sy;
                ToScreen(pt.x, pt.y, sx, sy);
                screen.push_back({sx, sy});
            }

            if (p.filled)
            {
                // Fan triangulation from vertex 0 - correct for convex
                // polygons only. Concave shapes will render wrong.
                std::vector<SDL_Vertex> verts;
                verts.reserve(screen.size());
                SDL_FColor col{p.color.r, p.color.g, p.color.b, p.color.a};
                for (auto& s : screen)
                    verts.push_back(SDL_Vertex{s, col, {0, 0}});

                std::vector<int> indices;
                for (size_t i = 1; i + 1 < screen.size(); ++i)
                {
                    indices.push_back(0);
                    indices.push_back(static_cast<int>(i));
                    indices.push_back(static_cast<int>(i + 1));
                }
                SDL_RenderGeometry(m_Renderer,
                                   nullptr,
                                   verts.data(),
                                   static_cast<int>(verts.size()),
                                   indices.data(),
                                   static_cast<int>(indices.size()));
            }
            else
            {
                for (size_t i = 0; i < screen.size(); ++i)
                {
                    size_t next = (i + 1) % screen.size();
                    SDL_RenderLine(
                        m_Renderer, screen[i].x, screen[i].y, screen[next].x, screen[next].y);
                }
            }
        }

        void DrawText(const DrawCmd::Text& t) override
        {
            float sx, sy;
            ToScreen(t.x, t.y, sx, sy);
            m_TextCache.Draw(t.font, t.str, sx, sy, t.size, t.color);
        }

        void DrawSprite(const DrawCmd::Sprite& s) override
        {
            auto it = m_Textures.find(s.textureHandle);
            if (it == m_Textures.end())
                return; // invalid/unloaded handle - skip rather than crash

            SDL_Texture* texture = it->second.texture;
            const TextureDesc& desc = it->second.desc;

            float w = (s.w > 0.0f ? s.w : static_cast<float>(desc.width)) * m_CamZoom;
            float h = (s.h > 0.0f ? s.h : static_cast<float>(desc.height)) * m_CamZoom;

            float sx, sy;
            ToScreen(s.x, s.y, sx, sy);
            SDL_FRect dst{sx - w * 0.5f, sy - h * 0.5f, w, h};

            // SDL_FlipMode is a bitmask despite being an enum - both flags
            // at once is a valid combination.
            int flip = SDL_FLIP_NONE;
            if (s.flipX)
                flip |= SDL_FLIP_HORIZONTAL;
            if (s.flipY)
                flip |= SDL_FLIP_VERTICAL;

            // Color/alpha mod is state on the SDL_Texture itself, not
            // per-draw-call - set fresh every time so one sprite's tint
            // can't bleed into the next draw of the same texture.
            SDL_SetTextureColorModFloat(texture, s.color.r, s.color.g, s.color.b);
            SDL_SetTextureAlphaModFloat(texture, s.color.a);

            SDL_RenderTextureRotated(m_Renderer,
                                     texture,
                                     nullptr,
                                     &dst,
                                     s.rotation,
                                     nullptr,
                                     static_cast<SDL_FlipMode>(flip));
        }

        void SetCamera(const DrawCmd::SetCamera& cam) override
        {
            m_CamX = cam.x;
            m_CamY = cam.y;
            m_CamZoom = cam.zoom;
        }

        void ResetCamera(const DrawCmd::ResetCamera&) override
        {
            m_CamX = 0.f;
            m_CamY = 0.f;
            m_CamZoom = 1.f;
        }

        void DrawFullscreenTexture(TextureHandle handle) override
        {
            auto it = m_Textures.find(handle);
            if (it == m_Textures.end())
                return;
            SDL_RenderTexture(m_Renderer, it->second.texture, nullptr, nullptr);
        }

        TextureHandle CreateTextureFromFile(const std::string& path) override
        {
            SDL_Texture* texture = IMG_LoadTexture(m_Renderer, path.c_str());
            if (!texture)
                return k_InvalidTexture;

            float w = 0.f, h = 0.f;
            SDL_GetTextureSize(texture, &w, &h);

            TextureHandle handle = m_NextHandle++;
            m_Textures[handle] = {texture, {static_cast<int>(w), static_cast<int>(h)}};
            return handle;
        }

        TextureDesc GetTextureDesc(TextureHandle handle) const override
        {
            auto it = m_Textures.find(handle);
            return it != m_Textures.end() ? it->second.desc : TextureDesc{};
        }

        void DestroyTexture(TextureHandle handle) override
        {
            auto it = m_Textures.find(handle);
            if (it == m_Textures.end())
                return;
            SDL_DestroyTexture(it->second.texture);
            m_Textures.erase(it);
        }

        void* GetNativeTextureHandle(TextureHandle handle) const override
        {
            auto it = m_Textures.find(handle);
            return it != m_Textures.end() ? static_cast<void*>(it->second.texture) : nullptr;
        }

      private:
        void ToScreen(float wx, float wy, float& sx, float& sy) const
        {
            sx = (wx - m_CamX) * m_CamZoom + m_ScreenWidth * 0.5f;
            sy = (wy - m_CamY) * m_CamZoom + m_ScreenHeight * 0.5f;
        }

        void SetColor(const Color& c)
        {
            auto to8 = [](float v) { return static_cast<Uint8>(std::clamp(v, 0.f, 1.f) * 255.f); };
            SDL_SetRenderDrawColor(m_Renderer, to8(c.r), to8(c.g), to8(c.b), to8(c.a));
        }

        SDL_Renderer* m_Renderer;
        TextCache& m_TextCache;

        float m_ScreenWidth{0.f};
        float m_ScreenHeight{0.f};

        float m_CamX{0.f}, m_CamY{0.f}, m_CamZoom{1.f};

        struct StoredTexture
        {
            SDL_Texture* texture{nullptr};
            TextureDesc desc{};
        };
        std::unordered_map<TextureHandle, StoredTexture> m_Textures;
        TextureHandle m_NextHandle{1};
    };
} // namespace Cadmium
#endif // #define CADMIUM_SDL_RENDERER
