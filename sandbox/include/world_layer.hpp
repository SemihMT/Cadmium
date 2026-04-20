#ifndef SANDBOX_WORLD_LAYER_HPP
#define SANDBOX_WORLD_LAYER_HPP

#include <cadmium/core/layer.hpp>
#include <cadmium/core/event_bus.hpp>
#include "game_state.hpp"
#include "events.hpp"
#include <memory>
#include <random>
#include <SDL3/SDL.h>

namespace Sandbox
{
  class WorldLayer : public Cadmium::Layer
  {
  public:
    explicit WorldLayer(std::shared_ptr<GameState> state)
      : Cadmium::Layer("World")
      , m_State{std::move(state)}
      , m_Rng{std::random_device{}()}
    {}

    void OnAttach()      override;
    void OnDetach()      override;
    void OnEvent(SDL_Event& event) override;
    void OnFixedUpdate(float dt)   override;

  private:
    // Setup
    void InitShip();
    void SpawnWave(int wave);
    void SpawnAsteroid(float x, float y, float vx, float vy, int generation);
    void Restart();

    // Update
    void HandleInput(float dt);
    void UpdateShip(float dt);
    void UpdateBullets(float dt);
    void UpdateAsteroids(float dt);
    void CheckCollisions();
    void CheckWaveComplete();

    // Collision
    bool CircleOverlap(float ax, float ay, float ar,
                       float bx, float by, float br) const;
    bool IsOffScreen(float x, float y) const;
    bool IsSafeSpawnPosition(float x, float y, float radius) const;

    // Helpers
    void  FireBullet();
    void  SplitAsteroid(Asteroid& asteroid);
    float RandomFloat(float min, float max);
    float RandomEdgeX();
    float RandomEdgeY();

    std::shared_ptr<GameState> m_State;

    // Input state
    bool m_ThrustHeld{false};
    bool m_LeftHeld{false};
    bool m_RightHeld{false};

    // Fire rate
    float m_FireCooldown{0.0f};
    static constexpr float k_FireRate = 0.25f; // seconds between shots

    // Invincibility flash after restart
    float m_InvincibilityTimer{0.0f};
    static constexpr float k_InvincibilityDuration = 2.0f;
    static constexpr float k_BlinkInterval          = 0.1f;

    std::mt19937                          m_Rng;
    std::uniform_real_distribution<float> m_Dist{0.0f, 1.0f};

    // Subscriptions
    Cadmium::SubscriptionToken m_RestartToken;
    Cadmium::SubscriptionToken m_SpawnAsteroidToken;
    Cadmium::SubscriptionToken m_InvincibilityToken;
    Cadmium::SubscriptionToken m_DebrisToken;
  };

} // namespace Sandbox

#endif // SANDBOX_WORLD_LAYER_HPP
