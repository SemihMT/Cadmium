#ifndef CADMIUM_ENGINE_CONTEXT_HPP
#define CADMIUM_ENGINE_CONTEXT_HPP
#include <memory>
#include <string>
#include <cadmium/core/event_bus.hpp>
namespace Cadmium
{
  class Layer;
  class Scene;
  class IEngineContext
  {
  public:

    virtual ~IEngineContext() = default;

    virtual void RequestQuit() = 0;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual void PushLayer(std::unique_ptr<Layer> layer) = 0;
    virtual void PushOverlay(std::unique_ptr<Layer> layer) = 0;
    virtual void PopLayer(const std::string &name) = 0;
    virtual void PopOverlay(const std::string &name) = 0;
    virtual EventBus& GetEventBus() = 0;
    virtual void PushScene(std::unique_ptr<Scene> scene) = 0;
    virtual void PopScene() = 0;
    virtual void ReplaceScene(std::unique_ptr<Scene> scene) = 0;
  };
} // namespace Cadmium

#endif // CADMIUM_ENGINE_CONTEXT_HPP
