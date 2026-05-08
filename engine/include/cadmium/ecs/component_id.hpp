#ifndef CADMIUM_ECS_COMPONENT_ID_HPP
#define CADMIUM_ECS_COMPONENT_ID_HPP

#include <cstdint>

namespace Cadmium
{

  using ComponentID = uint32_t;

  inline ComponentID NextComponentID()
  {
    static ComponentID counter = 0;
    return counter++;
  }

  template <typename T>
  ComponentID GetComponentID()
  {
    static ComponentID id = NextComponentID();
    return id;
  }

} // namespace Cadmium
#endif // CADMIUM_ECS_COMPONENT_ID_HPP
