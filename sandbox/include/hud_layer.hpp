#ifndef SANDBOX_HUD_LAYER_HPP
#define SANDBOX_HUD_LAYER_HPP

#include <cadmium/core/layer.hpp>
#include <cadmium/core/event_bus.hpp>
#include "game_state.hpp"
#include "events.hpp"
#include <memory>
#include <string>
#include <SDL3/SDL.h>

namespace Sandbox
{
  class HUDLayer : public Cadmium::Layer
  {
  public:
    explicit HUDLayer(std::shared_ptr<GameState> state)
      : Cadmium::Layer("HUD")
      , m_State{std::move(state)}
    {}

    void OnAttach() override;
    void OnDetach() override;
    void OnRender(SDL_Renderer* renderer) override;
    void OnEvent(SDL_Event& event) override;

  private:
    void DrawScore(SDL_Renderer* renderer)   const;
    void DrawWave(SDL_Renderer* renderer)    const;
    void DrawGameOver(SDL_Renderer* renderer) const;

    // Minimal bitmap digit renderer
    void DrawDigit(SDL_Renderer* renderer,
                   int digit,
                   float x, float y,
                   float scale) const;

    void DrawNumber(SDL_Renderer* renderer,
                    int number,
                    float x, float y,
                    float scale) const;

    void DrawText(SDL_Renderer* renderer,
                  const char* text,
                  float x, float y,
                  float scale) const;

    // Segment display helpers
    void DrawSegment(SDL_Renderer* renderer,
                     float x, float y,
                     float scale,
                     bool horizontal,
                     float offX, float offY) const;

    std::shared_ptr<GameState> m_State;

    int   m_DisplayScore{0};
    int   m_Wave{1};

    Cadmium::SubscriptionToken m_ScoreToken;
    Cadmium::SubscriptionToken m_WaveToken;
    Cadmium::SubscriptionToken m_DeathToken;
  };

} // namespace Sandbox

#endif // SANDBOX_HUD_LAYER_HPP
