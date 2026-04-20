#include "stress_render_layer.hpp"
#include <cadmium/ecs/world.hpp>
#include <cadmium/ecs/components.hpp>

namespace Sandbox
{
  void StressRenderLayer::OnRender(SDL_Renderer* renderer)
  {
    auto& world = GetWorld();

    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    for (auto& [entity, transform] : world.Query<Cadmium::Transform>())
      SDL_RenderPoint(renderer, transform->x, transform->y);
  }

} // namespace Sandbox
