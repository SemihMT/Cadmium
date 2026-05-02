#ifndef CADMIUM_SCRIPT_RENDER_LAYER_HPP
#define CADMIUM_SCRIPT_RENDER_LAYER_HPP
#include <cadmium/core/draw_command_queue.hpp>
#include <cadmium/core/layer.hpp>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <numbers>

namespace Cadmium
{

// Sits in the layer stack wherever you want script rendering to appear.
// On each OnRender it drains the DrawCommandQueue and executes SDL calls.
// Camera transform is applied to all commands between SetCamera/ResetCamera.
class ScriptRenderLayer : public Layer
{
public:
    explicit ScriptRenderLayer(DrawCommandQueue& queue, TTF_Font* font)
        : Layer("ScriptRenderLayer"), m_Queue(queue), m_Font(font) {}

    void OnRender(SDL_Renderer* renderer) override
    {
        // Reset camera state for this frame
        m_CamX = 0.f; m_CamY = 0.f; m_CamZoom = 1.f;

        for (const auto& cmd : m_Queue.Commands())
        {
            std::visit([&](const auto& c) { Execute(renderer, c); }, cmd);
        }

        m_Queue.Clear();
    }

private:
    DrawCommandQueue& m_Queue;
    TTF_Font* m_Font{nullptr};

    // Camera state — reset each frame, modified by SetCamera commands
    float m_CamX    = 0.f;
    float m_CamY    = 0.f;
    float m_CamZoom = 1.f;

    // Transform a world point to screen space
    float ToScreenX(float x) const { return (x - m_CamX) * m_CamZoom; }
    float ToScreenY(float y) const { return (y - m_CamY) * m_CamZoom; }

    void SetColor(SDL_Renderer* r, const Color& c)
    {
        SDL_SetRenderDrawColorFloat(r, c.r, c.g, c.b, c.a);
    }

    // ── Command executors ─────────────────────────────────────────────────

    void Execute(SDL_Renderer* r, const DrawCmd::Line& c)
    {
        SetColor(r, c.color);
        SDL_RenderLine(r,
            ToScreenX(c.x1), ToScreenY(c.y1),
            ToScreenX(c.x2), ToScreenY(c.y2));
    }

    void Execute(SDL_Renderer* r, const DrawCmd::Rect& c)
    {
        SetColor(r, c.color);
        SDL_FRect rect {
            ToScreenX(c.x), ToScreenY(c.y),
            c.w * m_CamZoom, c.h * m_CamZoom
        };
        if (c.filled)
            SDL_RenderFillRect(r, &rect);
        else
            SDL_RenderRect(r, &rect);
    }

    void Execute(SDL_Renderer* r, const DrawCmd::Circle& c)
    {
        SetColor(r, c.color);

        float sx = ToScreenX(c.x);
        float sy = ToScreenY(c.y);
        float sr = c.radius * m_CamZoom;

        int segs = c.segments > 0 ? c.segments : std::max(12, (int)(sr * 0.5f));

        if (c.filled)
        {
            // Filled circle via horizontal scanlines — SDL has no fill circle
            for (float dy = -sr; dy <= sr; dy += 1.f)
            {
                float dx = std::sqrt(sr * sr - dy * dy);
                SDL_RenderLine(r, sx - dx, sy + dy, sx + dx, sy + dy);
            }
        }
        else
        {
            float prev_x = sx + sr;
            float prev_y = sy;
            for (int i = 1; i <= segs; ++i)
            {
                float angle = (float)i / (float)segs * 2.f * std::numbers::pi;
                float nx = sx + std::cos(angle) * sr;
                float ny = sy + std::sin(angle) * sr;
                SDL_RenderLine(r, prev_x, prev_y, nx, ny);
                prev_x = nx;
                prev_y = ny;
            }
        }
    }

    void Execute(SDL_Renderer *r, const DrawCmd::Polygon &c)
    {
      if (c.points.size() < 2)
        return;
      SetColor(r, c.color);

      for (size_t i = 0; i < c.points.size(); ++i)
      {
        const auto &p1 = c.points[i];
        const auto &p2 = c.points[(i + 1) % c.points.size()];
        SDL_RenderLine(r,
                       ToScreenX(p1.x), ToScreenY(p1.y),
                       ToScreenX(p2.x), ToScreenY(p2.y));
      }
    }

    void Execute(SDL_Renderer *r, const DrawCmd::Text &c)
    {
        if (!m_Font)
            return; // font not loaded yet, skip silently

        SDL_Color sdlColor{
            (Uint8)(c.color.r * 255),
            (Uint8)(c.color.g * 255),
            (Uint8)(c.color.b * 255),
            (Uint8)(c.color.a * 255)};

        SDL_Surface *surface = TTF_RenderText_Blended(m_Font, c.str.c_str(), 0, sdlColor);
        if (!surface)
        {
            SDL_Log("TTF surface failed: %s", SDL_GetError());
            return;
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
        SDL_DestroySurface(surface);
        if (!texture)
        {
            SDL_Log("Texture creation failed: %s", SDL_GetError());
            return;
        }

        float w, h;
        SDL_GetTextureSize(texture, &w, &h);

        float fontBaseSize = TTF_GetFontSize(m_Font);
        float sizeScale = (fontBaseSize > 0.0f) ? (c.size / fontBaseSize) : 1.0f;

        SDL_FRect dst{ToScreenX(c.x), ToScreenY(c.y), w * sizeScale  * m_CamZoom, h * sizeScale * m_CamZoom};
        SDL_RenderTexture(r, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }

    void Execute(SDL_Renderer *r, const DrawCmd::Sprite &c)
    {
      SDL_FRect placeholder{};
      placeholder.h = c.h;
      placeholder.w = c.w;
      placeholder.x = c.x;
      placeholder.y = c.y;

      SDL_SetRenderDrawColor(r, 0xff, 0x00, 0xff, 0xff);
      SDL_RenderRect(r,&placeholder);
    }

    void Execute(SDL_Renderer* r, const DrawCmd::SetCamera& c)
    {
        m_CamX    = c.x;
        m_CamY    = c.y;
        m_CamZoom = c.zoom;
    }

    void Execute(SDL_Renderer* r, const DrawCmd::ResetCamera&)
    {
        m_CamX = 0.f; m_CamY = 0.f; m_CamZoom = 1.f;
    }
};

} // namespace Cadmium
#endif // CADMIUM_SCRIPT_RENDER_LAYER_HPP
