// sandbox/debris_system.hpp
#pragma once

#include <cadmium/ecs/system.hpp>
#include <cadmium/ecs/components.hpp>
#include "components.hpp"

namespace Sandbox
{
  class DebrisSystem : public Cadmium::System
  {
  public:
    void OnUpdate(Cadmium::Registry& registry, float dt) override
    {
      for (auto entity : registry.QueryEntities<Cadmium::Transform,
                                                Cadmium::Velocity,
                                                Debris>())
      {
        auto& transform = registry.GetComponent<Cadmium::Transform>(entity);
        auto& velocity  = registry.GetComponent<Cadmium::Velocity>(entity);
        auto& debris    = registry.GetComponent<Debris>(entity);

        transform.x += velocity.x * dt;
        transform.y += velocity.y * dt;

        debris.lifetime -= dt;
        debris.alpha     = std::max(0.0f, debris.lifetime / 2.0f);

        if (debris.lifetime <= 0.0f)
          registry.DestroyEntity(entity);
      }
    }
  };
}
