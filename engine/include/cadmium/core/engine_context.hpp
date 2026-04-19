#ifndef CADMIUM_ENGINE_CONTEXT_HPP
#define CADMIUM_ENGINE_CONTEXT_HPP
#include <memory>
#include <string>
namespace Cadmium
{
  class Layer;
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
  };
} // namespace Cadmium

#endif // CADMIUM_ENGINE_CONTEXT_HPP
