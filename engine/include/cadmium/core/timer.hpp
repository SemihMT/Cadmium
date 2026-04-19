#ifndef CADMIUM_TIMER_HPP
#define CADMIUM_TIMER_HPP

#include <SDL3/SDL.h>

namespace Cadmium
{
  struct Timer
  {
    Uint64 lastTime{SDL_GetPerformanceCounter()};
    Uint64 frequency{SDL_GetPerformanceFrequency()};

    float Peek() const;
    float DeltaTime();
    float DeltaTimeClamped(float maxDelta = 0.25f);
  };
} // namespace Cadmium

#endif // CADMIUM_TIMER_HPP
