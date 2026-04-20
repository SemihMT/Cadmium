#include <cadmium/core/layer_stack.hpp>
#include <cadmium/core/layer.hpp>
#include <algorithm>

namespace Cadmium
{
  void LayerStack::PushLayer(std::unique_ptr<Layer> layer, IEngineContext *context)
  {
    layer->SetContext(context);
    layer->OnAttach();
    m_Layers.emplace(m_Layers.begin() + m_OverlayIndex, std::move(layer));
    m_OverlayIndex++;
  }

  void LayerStack::PushOverlay(std::unique_ptr<Layer> layer, IEngineContext *context)
  {
    layer->SetContext(context);
    layer->OnAttach();
    m_Layers.emplace_back(std::move(layer));
  }

  void LayerStack::PopLayer(const std::string &name)
  {
    auto it = std::find_if(
        m_Layers.begin(),
        m_Layers.begin() + m_OverlayIndex,
        [&name](const auto &l)
        { return l->GetName() == name; });

    if (it != m_Layers.begin() + m_OverlayIndex)
    {
      (*it)->OnDetach();
      m_Layers.erase(it);
      m_OverlayIndex--;
    }
  }

  void LayerStack::PopOverlay(const std::string &name)
  {
    auto it = std::find_if(
        m_Layers.begin() + m_OverlayIndex,
        m_Layers.end(),
        [&name](const auto &l)
        { return l->GetName() == name; });

    if (it != m_Layers.end())
    {
      (*it)->OnDetach();
      m_Layers.erase(it);
    }
  }

  void LayerStack::RequestPushLayer(std::unique_ptr<Layer> layer)
  {
    m_Pending.push(PushLayerCmd{std::move(layer)});
  }

  void LayerStack::RequestPushOverlay(std::unique_ptr<Layer> layer)
  {
    m_Pending.push(PushOverlayCmd{std::move(layer)});
  }

  void LayerStack::RequestPopLayer(const std::string &name)
  {
    m_Pending.push(PopLayerCmd{name});
  }

  void LayerStack::RequestPopOverlay(const std::string &name)
  {
    m_Pending.push(PopOverlayCmd{name});
  }

  void LayerStack::FlushPending(IEngineContext *context)
  {
    while (!m_Pending.empty())
    {
      std::visit([&](auto &&cmd)
                 {
      using T = std::decay_t<decltype(cmd)>;
      if constexpr (std::is_same_v<T, PushLayerCmd>)
        PushLayer(std::move(cmd.layer), context);
      else if constexpr (std::is_same_v<T, PushOverlayCmd>)
        PushOverlay(std::move(cmd.layer), context);
      else if constexpr (std::is_same_v<T, PopLayerCmd>)
        PopLayer(cmd.name);
      else if constexpr (std::is_same_v<T, PopOverlayCmd>)
        PopOverlay(cmd.name); }, m_Pending.front());

      m_Pending.pop();
    }
  }

  void LayerStack::Clear()
  {
    for (auto &layer : m_Layers)
      layer->OnDetach();
    m_Layers.clear();
    m_OverlayIndex = 0;
  }
}
