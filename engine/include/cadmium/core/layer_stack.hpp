#ifndef CADMIUM_LAYER_STACK_HPP
#define CADMIUM_LAYER_STACK_HPP

#include <cadmium/core/layer.hpp>
#include <vector>
#include <memory>
#include <variant>
#include <queue>

namespace Cadmium
{
  class LayerStack
  {
  public:
    void PushLayer(std::unique_ptr<Layer> layer, IEngineContext* context);
    void PushOverlay(std::unique_ptr<Layer> layer, IEngineContext* context);
    void PopLayer(const std::string& name);
    void PopOverlay(const std::string& name);
    void Clear();

    // Bottom to top - for update and render
    auto begin() { return m_Layers.begin(); }
    auto end() { return m_Layers.end(); }

    // Top to bottom - for events
    auto rbegin() { return m_Layers.rbegin(); }
    auto rend() { return m_Layers.rend(); }

    // Called by Engine at the end of each Iterate()
    void FlushPending(IEngineContext *context);

    // Called by IEngineContext implementations instead of Push/Pop directly
    void RequestPushLayer(std::unique_ptr<Layer> layer);
    void RequestPushOverlay(std::unique_ptr<Layer> layer);
    void RequestPopLayer(const std::string &name);
    void RequestPopOverlay(const std::string &name);

  private:

    struct PushLayerCmd    { std::unique_ptr<Layer> layer; };
    struct PushOverlayCmd  { std::unique_ptr<Layer> layer; };
    struct PopLayerCmd     { std::string name; };
    struct PopOverlayCmd   { std::string name; };

    using Command = std::variant<PushLayerCmd, PushOverlayCmd, PopLayerCmd, PopOverlayCmd>;

    std::vector<std::unique_ptr<Layer>> m_Layers;
    std::queue<Command> m_Pending;
    size_t m_OverlayIndex{0}; // keeps track of where overlays begin
  };
} // namespace Cadmium

#endif // CADMIUM_LAYER_STACK_HPP
