#include <cadmium/core/event_bus.hpp>
#include <algorithm>

namespace Cadmium
{
  void SubscriptionToken::Reset()
  {
    if (m_Bus)
    {
      m_Bus->Unsubscribe(m_Type, m_Id);
      m_Bus = nullptr;
    }
  }

  void EventBus::Dispatch()
  {
    // Swap to allow posting during dispatch without infinite loop
    std::vector<std::function<void()>> toDispatch;
    std::swap(toDispatch, m_Pending);
    for (auto& dispatch : toDispatch)
      dispatch();
  }

  void EventBus::Unsubscribe(std::type_index type, uint64_t id)
  {
    auto it = m_Subscribers.find(type);
    if (it == m_Subscribers.end()) return;

    auto& subs = it->second;
    std::erase_if(subs, [id](const Subscriber& s){ return s.id == id; });

    if (subs.empty())
      m_Subscribers.erase(it);
  }

} // namespace Cadmium
