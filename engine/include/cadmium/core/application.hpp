#ifndef CADMIUM_APPLICATION_H
#define CADMIUM_APPLICATION_H
#include <SDL3/SDL.h>

namespace Cadmium
{

  // Generic "Application" class for the user.
  // Enables the user to be notified of engine processes
  class Application
  {
  public:
    virtual ~Application() = default;

    virtual void OnStart() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnFixedUpdate(float dt) {}
    virtual void OnRender() = 0;
    virtual void OnEvent(SDL_Event &event) {}
    virtual void OnShutdown() {}

    void Quit() { m_Running = false; }
    bool IsRunning() const { return m_Running; }

  private:
    bool m_Running{true};
  };

} // namespace Cadmium

#endif // CADMIUM_APPLICATION_H
