#ifndef CADMIUM_EVENT_BUS_HPP
#define CADMIUM_EVENT_BUS_HPP

#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace Cadmium
{
  class EventBus;

  // RAII for subscriptions. Prevents dangling listener pointers
  class SubscriptionToken
  {
  public:
    SubscriptionToken() = default;

    SubscriptionToken(EventBus* bus, std::type_index type, uint64_t id)
      : m_Bus(bus), m_Type(type), m_Id(id) {}

    ~SubscriptionToken() { Reset(); }

    SubscriptionToken(const SubscriptionToken&) = delete;
    SubscriptionToken& operator=(const SubscriptionToken&) = delete;

    SubscriptionToken(SubscriptionToken&& other) noexcept
      : m_Bus(other.m_Bus), m_Type(other.m_Type), m_Id(other.m_Id)
    {
      other.m_Bus = nullptr;
    }

    SubscriptionToken& operator=(SubscriptionToken&& other) noexcept
    {
      if (this != &other)
      {
        Reset();
        m_Bus  = other.m_Bus;
        m_Type = other.m_Type;
        m_Id   = other.m_Id;
        other.m_Bus = nullptr;
      }
      return *this;
    }

  private:
    void Reset();

    EventBus*       m_Bus{nullptr};
    std::type_index m_Type{typeid(void)};
    uint64_t        m_Id{0};
  };

  class EventBus
  {
  public:

    template<typename T>
    void Post(T event)
    {
      m_Pending.emplace_back(
        [this, event = std::move(event)]() mutable
        {
          auto it = m_Subscribers.find(std::type_index(typeid(T)));
          if (it == m_Subscribers.end()) return;

          for (auto& sub : it->second)
            sub.handler(&event);
        }
      );
    }

    template<typename T>
    SubscriptionToken Subscribe(std::function<void(const T&)> handler)
    {
      auto type = std::type_index(typeid(T));
      uint64_t id = m_NextId++;

      m_Subscribers[type].push_back({
        id,
        [handler = std::move(handler)](const void* ptr)
        {
          handler(*static_cast<const T*>(ptr));
        }
      });

      return SubscriptionToken(this, type, id);
    }

    void Dispatch();
    void Unsubscribe(std::type_index type, uint64_t id);

  private:
    struct Subscriber
    {
      uint64_t id;
      std::function<void(const void*)> handler;
    };

    std::unordered_map<std::type_index, std::vector<Subscriber>> m_Subscribers;
    std::vector<std::function<void()>> m_Pending;

    uint64_t m_NextId{1};

    friend class SubscriptionToken;
  };
} // namespace Cadmium

#endif
