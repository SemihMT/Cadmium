#ifndef SANDBOX_ENTITIES_HPP
#define SANDBOX_ENTITIES_HPP

namespace Sandbox
{
  struct Ship
  {
    float x, y;
    float vx{0.0f}, vy{0.0f};
    float angle{0.0f};
    bool  invincible{false};
    bool  alive{true};

    // Blink state for invincibility visual
    float blinkTimer{0.0f};
    bool  visible{true};

    static constexpr float k_Radius        = 16.0f;
    static constexpr float k_Thrust        = 400.0f;
    static constexpr float k_RotationSpeed = 180.0f; // degrees per second
    static constexpr float k_Damping       = 0.99f;
    static constexpr float k_MaxSpeed      = 600.0f;
  };

  struct Asteroid
  {
    float x, y;
    float vx, vy;
    float angle{0.0f};
    float rotationSpeed{0.0f};
    int   generation{0};     // 0 = large, 1 = medium, 2 = small
    bool  active{true};

    static constexpr float k_Radii[3] = { 60.0f, 30.0f, 15.0f };
    float Radius() const { return k_Radii[generation]; }
  };

  struct Bullet
  {
    float x, y;
    float vx, vy;
    float life{0.0f};
    bool  active{false};

    static constexpr float k_Speed  = 700.0f;
    static constexpr float k_Life   = 1.5f;
    static constexpr float k_Radius = 3.0f;
  };

} // namespace Sandbox

#endif // SANDBOX_ENTITIES_HPP
