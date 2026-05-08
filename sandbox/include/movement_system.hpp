#pragma once

#include <cadmium/ecs/system.hpp>
#include <cadmium/ecs/components.hpp>

namespace Sandbox
{
  class MovementSystem : public Cadmium::System
  {
  public:
    void OnUpdate(Cadmium::World& world, float dt) override
    {
      float width  = static_cast<float>(m_Width);
      float height = static_cast<float>(m_Height);

      for (auto entity : world.QueryEntities<Cadmium::Transform,
                                                Cadmium::Velocity>())
      {
        auto& transform = world.GetComponent<Cadmium::Transform>(entity);
        auto& velocity  = world.GetComponent<Cadmium::Velocity>(entity);

        transform.position.x += velocity.x * dt;
        transform.position.y += velocity.y * dt;

        // Wrap around screen
        if (transform.position.x < 0.0f)    transform.position.x += width;
        if (transform.position.x > width)   transform.position.x -= width;
        if (transform.position.y < 0.0f)    transform.position.y += height;
        if (transform.position.y > height)  transform.position.y -= height;
      }
    }

    void SetBounds(int width, int height)
    {
      m_Width  = width;
      m_Height = height;
    }

  private:
    int m_Width{1280};
    int m_Height{720};
  };

} // namespace Sandbox
