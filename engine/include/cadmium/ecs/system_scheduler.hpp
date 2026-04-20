#ifndef CADMIUM_SYSTEM_SCHEDULER_HPP
#define CADMIUM_SYSTEM_SCHEDULER_HPP

#include <cadmium/ecs/system.hpp>
#include <memory>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>

namespace Cadmium
{
  class SystemScheduler
  {
  public:
    template<typename T, typename... Args>
    T& RegisterSystem(int order, Args&&... args)
    {
      static_assert(std::is_base_of_v<System, T>,
                    "T must derive from System");

      auto type = std::type_index(typeid(T));
      if (m_Index.contains(type))
        throw std::runtime_error("System already registered");

      auto system  = std::make_unique<T>(std::forward<Args>(args)...);
      T*   ptr     = system.get();
      ptr->m_Order = order;

      m_Index[type] = ptr;
      m_Systems.push_back(std::move(system));

      // Keep sorted by order
      std::sort(m_Systems.begin(), m_Systems.end(),
        [](const auto& a, const auto& b)
        {
          return a->GetOrder() < b->GetOrder();
        });

      return *ptr;
    }

    template<typename T>
    T& GetSystem()
    {
      auto it = m_Index.find(typeid(T));
      if (it == m_Index.end())
        throw std::runtime_error("System not found");
      return *static_cast<T*>(it->second);
    }

    template<typename T>
    bool HasSystem() const
    {
      return m_Index.contains(typeid(T));
    }

    template<typename T>
    void UnregisterSystem(Registry& registry)
    {
      auto type = std::type_index(typeid(T));
      auto it   = m_Index.find(type);
      if (it == m_Index.end()) return;

      it->second->OnStop(registry);
      m_Index.erase(it);

      m_Systems.erase(
        std::remove_if(m_Systems.begin(), m_Systems.end(),
          [&type](const auto& s)
          {
            return std::type_index(typeid(*s)) == type;
          }),
        m_Systems.end());
    }

    void Start(Registry& registry)
    {
      for (auto& system : m_Systems)
        system->OnStart(registry);
    }

    void Update(Registry& registry, float dt)
    {
      for (auto& system : m_Systems)
        system->OnUpdate(registry, dt);
    }

    void Stop(Registry& registry)
    {
      // Stop in reverse order
      for (auto it = m_Systems.rbegin(); it != m_Systems.rend(); ++it)
        (*it)->OnStop(registry);
    }

  private:
    std::vector<std::unique_ptr<System>>        m_Systems;
    std::unordered_map<std::type_index, System*> m_Index;
  };

} // namespace Cadmium

#endif // CADMIUM_SYSTEM_SCHEDULER_HPP
