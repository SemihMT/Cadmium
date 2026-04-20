#pragma once

#include <cadmium/ecs/system.hpp>
#include <cadmium/ecs/components.hpp>

namespace Sandbox
{
  class MovementSystem : public Cadmium::System
  {
  public:
    void OnUpdate(Cadmium::Registry& registry, float dt) override
    {
      float width  = static_cast<float>(m_Width);
      float height = static_cast<float>(m_Height);

      for (auto entity : registry.QueryEntities<Cadmium::Transform,
                                                Cadmium::Velocity>())
      {
        auto& transform = registry.GetComponent<Cadmium::Transform>(entity);
        auto& velocity  = registry.GetComponent<Cadmium::Velocity>(entity);

        transform.x += velocity.x * dt;
        transform.y += velocity.y * dt;

        // Wrap around screen
        if (transform.x < 0.0f)    transform.x += width;
        if (transform.x > width)   transform.x -= width;
        if (transform.y < 0.0f)    transform.y += height;
        if (transform.y > height)  transform.y -= height;
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
