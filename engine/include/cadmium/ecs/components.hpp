#ifndef CADMIUM_COMPONENTS_HPP
#define CADMIUM_COMPONENTS_HPP

#include <string>

namespace Cadmium
{
  struct Transform
  {
    float x{0.0f}, y{0.0f};
    float rotation{0.0f};
    float scaleX{1.0f}, scaleY{1.0f};
  };

  struct Velocity
  {
    float x{0.0f}, y{0.0f};
  };

  struct Tag
  {
    std::string name;
  };

} // namespace Cadmium

#endif // CADMIUM_COMPONENTS_HPP
