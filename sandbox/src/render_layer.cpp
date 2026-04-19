#include "render_layer.hpp"
#include <cmath>
#include <numbers>

namespace Sandbox
{
  static constexpr float k_Pi      = std::numbers::pi_v<float>;
  static constexpr float k_Deg2Rad = k_Pi / 180.0f;

  void RenderLayer::OnRender(SDL_Renderer* renderer)
  {
    DrawAsteroids(renderer);
    DrawBullets(renderer);

    if (m_State->ship.alive && m_State->ship.visible)
    {
      DrawThrust(renderer);
      DrawShip(renderer);
    }
  }

  void RenderLayer::DrawShip(SDL_Renderer* renderer) const
  {
    const auto& ship = m_State->ship;

    // Local space (unrotated, pointing up)
    constexpr int k_Points = 3;
    float lx[k_Points] = {  0.0f, -10.0f,  10.0f };
    float ly[k_Points] = { -16.0f,  14.0f,  14.0f };

    float wx[k_Points], wy[k_Points];
    for (int i = 0; i < k_Points; i++)
      RotatePoint(lx[i], ly[i], 0.0f, 0.0f, ship.angle, wx[i], wy[i]);

    // Translate to world position
    for (int i = 0; i < k_Points; i++)
    {
      wx[i] += ship.x;
      wy[i] += ship.y;
    }

    // White ship
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    DrawPolygon(renderer, wx, wy, k_Points);

    // Draw engine nozzle detail
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    float nx1, ny1, nx2, ny2;
    RotatePoint(-5.0f, 12.0f, 0.0f, 0.0f, ship.angle, nx1, ny1);
    RotatePoint( 5.0f, 12.0f, 0.0f, 0.0f, ship.angle, nx2, ny2);
    DrawLine(renderer,
             ship.x + nx1, ship.y + ny1,
             ship.x + nx2, ship.y + ny2);
  }

  void RenderLayer::DrawThrust(SDL_Renderer* renderer) const
  {
    const auto& ship = m_State->ship;

    // Flame tip in local space
    float fx, fy;
    RotatePoint(0.0f, 28.0f, 0.0f, 0.0f, ship.angle, fx, fy);

    float lx, ly, rx, ry;
    RotatePoint(-5.0f, 14.0f, 0.0f, 0.0f, ship.angle, lx, ly);
    RotatePoint( 5.0f, 14.0f, 0.0f, 0.0f, ship.angle, rx, ry);

    SDL_SetRenderDrawColor(renderer, 255, 140, 0, 200);
    DrawLine(renderer,
             ship.x + lx, ship.y + ly,
             ship.x + fx, ship.y + fy);
    DrawLine(renderer,
             ship.x + rx, ship.y + ry,
             ship.x + fx, ship.y + fy);
  }

  void RenderLayer::DrawAsteroids(SDL_Renderer* renderer) const
  {
    // Color by generation
    static constexpr SDL_Color k_Colors[3] = {
      { 180, 160, 140, 255 }, // large  - warm grey
      { 140, 180, 140, 255 }, // medium - muted green
      { 140, 160, 200, 255 }, // small  - cool blue
    };

    for (const auto& a : m_State->asteroids)
    {
      if (!a.active) continue;

      const auto& c = k_Colors[a.generation];
      SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

      // Asteroids are irregular polygons approximated by a rotated circle
      // with per-vertex radius variation baked from generation seed
      constexpr int k_Segments = 10;
      float xs[k_Segments], ys[k_Segments];

      // Use asteroid pointer as a stable seed for shape variation
      // so each asteroid has a consistent irregular shape
      uint64_t seed = reinterpret_cast<uint64_t>(&a);

      for (int i = 0; i < k_Segments; i++)
      {
        float segAngle = (static_cast<float>(i) / k_Segments)
                       * 2.0f * k_Pi + a.angle * k_Deg2Rad;

        // Pseudo-random radius variation per vertex
        uint64_t h = seed ^ (static_cast<uint64_t>(i) * 2654435761ULL);
        float variation = 0.75f + 0.25f * static_cast<float>(h % 100) / 100.0f;
        float r = a.Radius() * variation;

        float lx = std::cos(segAngle) * r;
        float ly = std::sin(segAngle) * r;
        xs[i] = a.x + lx;
        ys[i] = a.y + ly;
      }

      DrawPolygon(renderer, xs, ys, k_Segments);
    }
  }

  void RenderLayer::DrawBullets(SDL_Renderer* renderer) const
  {
    SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
    for (const auto& b : m_State->bullets)
    {
      if (!b.active) continue;
      DrawCircle(renderer, b.x, b.y, Bullet::k_Radius, 6);
    }
  }
  // Primitives
  // TODO: Move these into a Cadmium::Utils::Drawing namespace
  void RenderLayer::DrawLine(SDL_Renderer* renderer,
                              float x1, float y1,
                              float x2, float y2) const
  {
    SDL_RenderLine(renderer, x1, y1, x2, y2);
  }

  void RenderLayer::DrawCircle(SDL_Renderer* renderer,
                                float cx, float cy,
                                float radius,
                                int segments) const
  {
    float prev_x = cx + radius;
    float prev_y = cy;

    for (int i = 1; i <= segments; i++)
    {
      float angle = (static_cast<float>(i) / segments) * 2.0f * k_Pi;
      float nx    = cx + std::cos(angle) * radius;
      float ny    = cy + std::sin(angle) * radius;
      SDL_RenderLine(renderer, prev_x, prev_y, nx, ny);
      prev_x = nx;
      prev_y = ny;
    }
  }

  void RenderLayer::DrawPolygon(SDL_Renderer* renderer,
                                 const float* xs, const float* ys,
                                 int count) const
  {
    for (int i = 0; i < count; i++)
    {
      int next = (i + 1) % count;
      SDL_RenderLine(renderer, xs[i], ys[i], xs[next], ys[next]);
    }
  }

  void RenderLayer::RotatePoint(float px, float py,
                                 float cx, float cy,
                                 float angleDeg,
                                 float& outX, float& outY) const
  {
    float rad  = -angleDeg * k_Deg2Rad;
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);
    float dx   = px - cx;
    float dy   = py - cy;
    outX = cx + dx * cosA - dy * sinA;
    outY = cy + dx * sinA + dy * cosA;
  }

} // namespace Sandbox
