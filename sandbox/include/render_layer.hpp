#ifndef SANDBOX_RENDER_LAYER_HPP
#define SANDBOX_RENDER_LAYER_HPP

#include <cadmium/core/layer.hpp>
#include "game_state.hpp"
#include <memory>
#include <SDL3/SDL.h>

namespace Sandbox
{
  class RenderLayer : public Cadmium::Layer
  {
  public:
    explicit RenderLayer(std::shared_ptr<GameState> state)
      : Cadmium::Layer("Render")
      , m_State{std::move(state)}
    {}

    void OnRender(SDL_Renderer* renderer) override;

  private:
    void DrawShip(SDL_Renderer* renderer)      const;
    void DrawAsteroids(SDL_Renderer* renderer) const;
    void DrawBullets(SDL_Renderer* renderer)   const;
    void DrawThrust(SDL_Renderer* renderer)    const;

    // Drawing primitives
    void DrawLine(SDL_Renderer* renderer,
                  float x1, float y1,
                  float x2, float y2) const;

    void DrawCircle(SDL_Renderer* renderer,
                    float cx, float cy,
                    float radius,
                    int segments = 16) const;

    void DrawPolygon(SDL_Renderer* renderer,
                     const float* xs, const float* ys,
                     int count) const;

    // Rotate a point around an origin
    void RotatePoint(float px, float py,
                     float cx, float cy,
                     float angleDeg,
                     float& outX, float& outY) const;

    std::shared_ptr<GameState> m_State;
  };

} // namespace Sandbox

#endif // SANDBOX_RENDER_LAYER_HPP
