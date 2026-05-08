#pragma once

#include <cadmium/ecs/system.hpp>
#include <cadmium/ecs/components.hpp>
#include "components.hpp"

namespace Sandbox
{
  class DebrisSystem : public Cadmium::System
  {
  public:
    void OnUpdate(Cadmium::World& world, float dt) override
    {
      for (auto entity : world.QueryEntities<Cadmium::Transform,
                                                Cadmium::Velocity,
                                                Debris>())
      {
        auto& transform = world.GetComponent<Cadmium::Transform>(entity);
        auto& velocity  = world.GetComponent<Cadmium::Velocity>(entity);
        auto& debris    = world.GetComponent<Debris>(entity);

        transform.position.x += velocity.x * dt;
        transform.position.y += velocity.y * dt;

        debris.lifetime -= dt;
        debris.alpha     = std::max(0.0f, debris.lifetime / 2.0f);

        if (debris.lifetime <= 0.0f)
          world.DestroyEntity(entity);
      }
    }
  };
}
