#ifndef CADMIUM_SYSTEM_HPP
#define CADMIUM_SYSTEM_HPP

#include <cadmium/ecs/registry.hpp>

namespace Cadmium
{
  class System
  {
  public:
    virtual ~System() = default;

    virtual void OnStart(Registry&)           {}
    virtual void OnUpdate(Registry&, float dt) = 0;
    virtual void OnStop(Registry&)            {}

    int GetOrder() const { return m_Order; }

  private:
    friend class SystemScheduler;
    int m_Order{0};
  };

} // namespace Cadmium

#endif // CADMIUM_SYSTEM_HPP
