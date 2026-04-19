#ifndef CADMIUM_APPLICATION_HPP
#define CADMIUM_APPLICATION_HPP

#include <cadmium/core/engine_context.hpp>
#include <SDL3/SDL.h>

namespace Cadmium
{
  class Application
  {
  public:
    virtual ~Application() = default;

    virtual void OnStart() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnFixedUpdate(float dt) {}
    virtual void OnRender(SDL_Renderer *renderer) = 0;
    virtual void OnEvent(SDL_Event &event) {}
    virtual void OnShutdown() {}
    virtual void OnImGuiRender() {}

    void SetContext(IEngineContext *context) { m_Context = context; }

  protected:
    void Quit() { m_Context->RequestQuit(); }
    int GetWidth() const { return m_Context->GetWidth(); }
    int GetHeight() const { return m_Context->GetHeight(); }

  private:
    IEngineContext *m_Context{nullptr};
  };
} // namespace Cadmium

#endif // CADMIUM_APPLICATION_HPP
