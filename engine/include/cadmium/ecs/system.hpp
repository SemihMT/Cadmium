#ifndef CADMIUM_SYSTEM_HPP
#define CADMIUM_SYSTEM_HPP

#include <cadmium/ecs/registry.hpp>

namespace Cadmium
{
  class World;
  class System
  {
  public:
    virtual ~System() = default;

    virtual void OnStart(World&)           {}
    virtual void OnUpdate(World&, float dt) = 0;
    virtual void OnStop(World&)            {}

    int GetOrder() const { return m_Order; }

  private:
    friend class SystemScheduler;
    int m_Order{0};
  };

} // namespace Cadmium

#endif // CADMIUM_SYSTEM_HPP
