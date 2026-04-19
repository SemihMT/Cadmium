#ifndef CADMIUM_ENGINE_CONTEXT_HPP
#define CADMIUM_ENGINE_CONTEXT_HPP

namespace Cadmium
{
  class IEngineContext
  {
  public:
    virtual ~IEngineContext() = default;

    virtual void RequestQuit() = 0;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
  };
} // namespace Cadmium

#endif // CADMIUM_ENGINE_CONTEXT_HPP
