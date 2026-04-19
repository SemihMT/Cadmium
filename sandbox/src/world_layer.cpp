#include "world_layer.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

using namespace Cadmium;
namespace Sandbox
{
  static constexpr float k_Pi = std::numbers::pi_v<float>;
  static constexpr float k_Deg2Rad = k_Pi / 180.0f;

  void WorldLayer::OnAttach()
  {

    m_RestartToken = Subscribe<RestartEvent>([this](const RestartEvent&)
    {
      SDL_Log("RestartEvent received");
      Restart();
    });

    m_SpawnAsteroidToken = Subscribe<SpawnAsteroidEvent>([this](const SpawnAsteroidEvent&)
    {
      float x  = RandomEdgeX();
      float y  = RandomEdgeY();
      float vx = RandomFloat(-80.0f, 80.0f);
      float vy = RandomFloat(-80.0f, 80.0f);
      SpawnAsteroid(x, y, vx, vy, 0);
    });

    m_InvincibilityToken = Subscribe<ToggleInvincibilityEvent>([this](const ToggleInvincibilityEvent&)
    {
      m_State->ship.invincible = !m_State->ship.invincible;
    });

    Restart();
  }

  void WorldLayer::OnDetach()
  {
    m_RestartToken       = {};
    m_SpawnAsteroidToken = {};
    m_InvincibilityToken = {};
  }

  void WorldLayer::Restart()
  {
    auto& s = *m_State;

    // Clear all entities
    for (auto& a : s.asteroids) a.active = false;
    for (auto& b : s.bullets)   b.active = false;
    s.asteroidCount = 0;
    s.bulletCount   = 0;
    s.gameOver      = false;
    s.score         = 0;
    s.wave          = 0;

    InitShip();

    s.wave = 1;
    SpawnWave(s.wave);

    Post(ScoreChangedEvent{ s.score });
    Post(WaveCompletedEvent{ s.wave });
  }

  void WorldLayer::InitShip()
  {
    auto& ship    = m_State->ship;
    ship.x        = GetWidth()  * 0.5f;
    ship.y        = GetHeight() * 0.5f;
    ship.vx       = 0.0f;
    ship.vy       = 0.0f;
    ship.angle    = 0.0f;
    ship.alive    = true;
    ship.visible  = true;
    ship.blinkTimer = 0.0f;

    // Brief invincibility on spawn
    ship.invincible      = true;
    m_InvincibilityTimer = k_InvincibilityDuration;
  }

  void WorldLayer::SpawnWave(int wave)
  {
    int count = 4 + (wave - 1) * 2;
    for (int i = 0; i < count; i++)
    {
      float x, y;
      float speed = RandomFloat(60.0f, 120.0f + wave * 10.0f);
      float angle = RandomFloat(0.0f, 2.0f * k_Pi);
      float vx    = std::cos(angle) * speed;
      float vy    = std::sin(angle) * speed;

      // Spawn on edges, away from ship
      do {
        x = RandomEdgeX();
        y = RandomEdgeY();
      } while (!IsSafeSpawnPosition(x, y, Asteroid::k_Radii[0]));

      SpawnAsteroid(x, y, vx, vy, 0);
    }
  }

  void WorldLayer::SpawnAsteroid(float x, float y, float vx, float vy, int generation)
  {
    auto& s = *m_State;
    if (s.asteroidCount >= GameState::k_MaxAsteroids) return;

    // Find inactive slot
    for (auto& a : s.asteroids)
    {
      if (a.active) continue;
      a.x              = x;
      a.y              = y;
      a.vx             = vx;
      a.vy             = vy;
      a.angle          = RandomFloat(0.0f, 360.0f);
      a.rotationSpeed  = RandomFloat(-90.0f, 90.0f);
      a.generation     = generation;
      a.active         = true;
      s.asteroidCount++;
      return;
    }
  }
void WorldLayer::OnEvent(SDL_Event& event)
  {
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
      switch (event.key.key)
      {
        case SDLK_LEFT:  m_LeftHeld   = true; break;
        case SDLK_RIGHT: m_RightHeld  = true; break;
        case SDLK_UP:    m_ThrustHeld = true; break;
        case SDLK_SPACE:
          if (!m_State->gameOver) FireBullet();
          break;
        case SDLK_R:
          Post(RestartEvent{});
          break;
        case SDLK_ESCAPE:
          Quit();
          break;
        default: break;
      }
    }

    if (event.type == SDL_EVENT_KEY_UP)
    {
      switch (event.key.key)
      {
        case SDLK_LEFT:  m_LeftHeld   = false; break;
        case SDLK_RIGHT: m_RightHeld  = false; break;
        case SDLK_UP:    m_ThrustHeld = false; break;
        default: break;
      }
    }
  }

  void WorldLayer::OnFixedUpdate(float dt)
  {
    if (m_State->gameOver) return;

    HandleInput(dt);
    UpdateShip(dt);
    UpdateBullets(dt);
    UpdateAsteroids(dt);
    CheckCollisions();
    CheckWaveComplete();

    if (m_FireCooldown > 0.0f)
      m_FireCooldown -= dt;
  }

  void WorldLayer::HandleInput(float dt)
  {
    auto& ship = m_State->ship;
    if (!ship.alive) return;

    if (m_LeftHeld)
      ship.angle += Ship::k_RotationSpeed * dt;
    if (m_RightHeld)
      ship.angle -= Ship::k_RotationSpeed * dt;

    if (m_ThrustHeld)
    {
      float rad = ship.angle * k_Deg2Rad;
      ship.vx  += -std::sin(rad) * Ship::k_Thrust * dt;
      ship.vy  += -std::cos(rad) * Ship::k_Thrust * dt;
    }
  }

  void WorldLayer::UpdateShip(float dt)
  {
    auto& ship = m_State->ship;
    if (!ship.alive) return;

    // Damping
    ship.vx *= std::pow(Ship::k_Damping, dt * 60.0f);
    ship.vy *= std::pow(Ship::k_Damping, dt * 60.0f);

    // Clamp speed
    float speed = std::sqrt(ship.vx * ship.vx + ship.vy * ship.vy);
    if (speed > Ship::k_MaxSpeed)
    {
      ship.vx = ship.vx / speed * Ship::k_MaxSpeed;
      ship.vy = ship.vy / speed * Ship::k_MaxSpeed;
    }

    ship.x += ship.vx * dt;
    ship.y += ship.vy * dt;

    // Wrap around screen edges
    float w = static_cast<float>(GetWidth());
    float h = static_cast<float>(GetHeight());
    if (ship.x < 0.0f)  ship.x += w;
    if (ship.x > w)     ship.x -= w;
    if (ship.y < 0.0f)  ship.y += h;
    if (ship.y > h)     ship.y -= h;

    // Invincibility timer
    if (ship.invincible && m_InvincibilityTimer > 0.0f)
    {
      m_InvincibilityTimer -= dt;
      ship.blinkTimer      += dt;

      if (ship.blinkTimer >= k_BlinkInterval)
      {
        ship.blinkTimer = 0.0f;
        ship.visible    = !ship.visible;
      }

      if (m_InvincibilityTimer <= 0.0f)
      {
        ship.invincible = false;
        ship.visible    = true;
      }
    }
  }

  void WorldLayer::UpdateBullets(float dt)
  {
    auto& s = *m_State;
    float w = static_cast<float>(GetWidth());
    float h = static_cast<float>(GetHeight());

    for (auto& b : s.bullets)
    {
      if (!b.active) continue;
      b.x += b.vx * dt;
      b.y += b.vy * dt;

      if (b.x < 0.0f || b.x > w || b.y < 0.0f || b.y > h)
      {
        b.active = false;
        s.bulletCount--;
      }
    }
  }

  void WorldLayer::UpdateAsteroids(float dt)
  {
    auto& s = *m_State;
    float w = static_cast<float>(GetWidth());
    float h = static_cast<float>(GetHeight());

    for (auto& a : s.asteroids)
    {
      if (!a.active) continue;
      a.x     += a.vx * dt;
      a.y     += a.vy * dt;
      a.angle += a.rotationSpeed * dt;

      // Wrap around screen
      if (a.x < -a.Radius())  a.x += w + a.Radius() * 2.0f;
      if (a.x >  w + a.Radius()) a.x -= w + a.Radius() * 2.0f;
      if (a.y < -a.Radius())  a.y += h + a.Radius() * 2.0f;
      if (a.y >  h + a.Radius()) a.y -= h + a.Radius() * 2.0f;
    }
  }
void WorldLayer::CheckCollisions()
  {
    auto& s = *m_State;

    for (auto& a : s.asteroids)
    {
      if (!a.active) continue;

      // Bullet vs asteroid
      for (auto& b : s.bullets)
      {
        if (!b.active) continue;
        if (!CircleOverlap(a.x, a.y, a.Radius(),
                           b.x, b.y, Bullet::k_Radius)) continue;

        b.active = false;
        s.bulletCount--;

        Post(AsteroidDestroyedEvent{ a.generation, a.x, a.y });
        SplitAsteroid(a);

        // Score: larger asteroids worth less
        int points = (2 - a.generation) * 100 + 100;
        s.score   += points;
        Post(ScoreChangedEvent{ s.score });
        break;
      }

      if (!a.active) continue;

      // Ship vs asteroid
      if (!s.ship.alive || s.ship.invincible) continue;
      if (!CircleOverlap(s.ship.x, s.ship.y, Ship::k_Radius,
                         a.x, a.y, a.Radius())) continue;

      s.ship.alive   = false;
      s.gameOver     = true;
      Post(PlayerDiedEvent{});
    }
  }

  void WorldLayer::SplitAsteroid(Asteroid& asteroid)
  {
    auto& s = *m_State;
    asteroid.active = false;
    s.asteroidCount--;

    if (asteroid.generation >= 2) return;

    int nextGen = asteroid.generation + 1;
    float speed = RandomFloat(80.0f, 160.0f);

    // Two children flying apart
    float angleA = RandomFloat(0.0f, 2.0f * k_Pi);
    float angleB = angleA + k_Pi + RandomFloat(-0.5f, 0.5f);

    SpawnAsteroid(
      asteroid.x, asteroid.y,
      std::cos(angleA) * speed + asteroid.vx * 0.5f,
      std::sin(angleA) * speed + asteroid.vy * 0.5f,
      nextGen);

    SpawnAsteroid(
      asteroid.x, asteroid.y,
      std::cos(angleB) * speed + asteroid.vx * 0.5f,
      std::sin(angleB) * speed + asteroid.vy * 0.5f,
      nextGen);
  }

  void WorldLayer::CheckWaveComplete()
  {
    if (m_State->asteroidCount > 0) return;

    m_State->wave++;
    SpawnWave(m_State->wave);
    Post(WaveCompletedEvent{ m_State->wave });
  }

  void WorldLayer::FireBullet()
  {
    auto& s = *m_State;
    if (!s.ship.alive)              return;
    if (m_FireCooldown > 0.0f)     return;
    if (s.bulletCount >= GameState::k_MaxBullets) return;

    float rad = s.ship.angle * k_Deg2Rad;
    float nx  = -std::sin(rad);
    float ny  = -std::cos(rad);

    for (auto& b : s.bullets)
    {
      if (b.active) continue;
      b.x      = s.ship.x + nx * Ship::k_Radius;
      b.y      = s.ship.y + ny * Ship::k_Radius;
      b.vx     = nx * Bullet::k_Speed + s.ship.vx;
      b.vy     = ny * Bullet::k_Speed + s.ship.vy;
      b.life   = Bullet::k_Life;
      b.active = true;
      s.bulletCount++;
      break;
    }

    m_FireCooldown = k_FireRate;
  }

  bool WorldLayer::CircleOverlap(float ax, float ay, float ar,
                                  float bx, float by, float br) const
  {
    float dx   = ax - bx;
    float dy   = ay - by;
    float rSum = ar + br;
    return (dx * dx + dy * dy) < (rSum * rSum);
  }

  bool WorldLayer::IsOffScreen(float x, float y) const
  {
    return x < 0.0f || x > GetWidth() || y < 0.0f || y > GetHeight();
  }

  bool WorldLayer::IsSafeSpawnPosition(float x, float y, float radius) const
  {
    auto& ship = m_State->ship;
    float dx   = x - ship.x;
    float dy   = y - ship.y;
    float minDist = radius + Ship::k_Radius + 100.0f;
    return (dx * dx + dy * dy) > (minDist * minDist);
  }

  float WorldLayer::RandomFloat(float min, float max)
  {
    return min + m_Dist(m_Rng) * (max - min);
  }

  float WorldLayer::RandomEdgeX()
  {
    float w = static_cast<float>(GetWidth());
    float h = static_cast<float>(GetHeight());
    if (m_Dist(m_Rng) < 0.5f)
      return RandomFloat(0.0f, w);
    return (m_Dist(m_Rng) < 0.5f) ? 0.0f : w;
  }

  float WorldLayer::RandomEdgeY()
  {
    float w = static_cast<float>(GetWidth());
    float h = static_cast<float>(GetHeight());
    if (m_Dist(m_Rng) < 0.5f)
      return (m_Dist(m_Rng) < 0.5f) ? 0.0f : h;
    return RandomFloat(0.0f, h);
  }

} // namespace Sandbox
