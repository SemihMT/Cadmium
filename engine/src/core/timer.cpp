#include "cadmium/core/timer.hpp"
#include <algorithm>

namespace Cadmium
{
  float Timer::DeltaTime()
  {
    Uint64 current = SDL_GetPerformanceCounter();
    float dt = (current - lastTime) / static_cast<float>(frequency);
    lastTime = current;
    return dt;
  }

  float Timer::DeltaTimeClamped(float maxDelta)
  {
    return std::min(DeltaTime(), maxDelta);
  }

} // namespace Cadmium
