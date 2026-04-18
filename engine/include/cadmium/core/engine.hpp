#ifndef CADMIUM_ENGINE_HPP
#define CADMIUM_ENGINE_HPP

#include <cadmium/core/application.hpp>
#include <cadmium/core/timer.hpp>
#include <SDL3/SDL.h>
#include <memory>

#ifdef CADMIUM_PLATFORM_WEB
#include <emscripten.h>
#endif

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#endif

namespace Cadmium
{
  class Engine
  {
  public:
    Engine(std::unique_ptr<Application> app, const char *title, int width, int height);
    ~Engine();

    void Run();

    SDL_Renderer *GetRenderer() const { return m_Renderer; }

  private:
    void Iterate();

#ifdef CADMIUM_IMGUI
    void InitImGui();
    void ShutdownImGui();
    void BeginImGuiFrame();
    void EndImGuiFrame();
#endif

    std::unique_ptr<Application> m_App{nullptr};
    SDL_Window *m_Window{nullptr};
    SDL_Renderer *m_Renderer{nullptr};
    Timer m_Timer{};

    static constexpr float m_FixedTimestep{1.0f / 60.0f};
    float m_Accumulator{0.0f};

#ifdef CADMIUM_PLATFORM_WEB
    static Engine *s_Instance;
    static void StaticIterate();
#endif
  };

} // namespace Cadmium
#endif // CADMIUM_ENGINE_HPP
