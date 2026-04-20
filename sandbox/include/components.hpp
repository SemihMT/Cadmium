#pragma once

#include <cadmium/ecs/components.hpp>

namespace Sandbox
{
  struct Debris
  {
    float radius{3.0f};
    float r{1.0f}, g{1.0f}, b{1.0f}; // color
    float alpha{1.0f};                // fades to 0
    float lifetime{2.0f};             // remaining life
  };

  struct StressTag {};
}
