#ifndef SANDBOX_GAME_STATE_HPP
#define SANDBOX_GAME_STATE_HPP

#include "entities.hpp"
#include <array>
#include <cstddef>

namespace Sandbox
{
  struct GameState
  {
    // Pre-allocated pools
    static constexpr size_t k_MaxAsteroids = 64;
    static constexpr size_t k_MaxBullets   = 32;

    Ship  ship{};
    std::array<Asteroid, k_MaxAsteroids> asteroids{};
    std::array<Bullet,   k_MaxBullets>   bullets{};

    size_t asteroidCount{0};
    size_t bulletCount{0};

    int  score{0};
    int  wave{0};
    bool gameOver{false};
  };

} // namespace Sandbox

#endif // SANDBOX_GAME_STATE_HPP
