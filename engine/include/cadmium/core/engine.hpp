#ifndef CADMIUM_ENGINE_HPP
#define CADMIUM_ENGINE_HPP

#include <cadmium/core/application.hpp>
#include <cadmium/core/engine_context.hpp>
#include <cadmium/core/imgui_layer.hpp>
#include <cadmium/core/timer.hpp>
#include <SDL3/SDL.h>
#include <memory>

#ifdef CADMIUM_PLATFORM_WEB
#include <emscripten.h>
#endif

namespace Cadmium
{
  class Engine : public IEngineContext
  {
  public:
    Engine(std::unique_ptr<Application> app, const char *title, int width, int height);
    ~Engine();

    void Run();

    void SetClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void SetVSync(bool enabled) { SDL_SetRenderVSync(m_Renderer, enabled ? 1 : 0); }
    void DisableDefaultBackground();

    SDL_Renderer *GetRenderer() const { return m_Renderer; }
    void SetTargetFPS(int fps) { m_TargetFrameTime = (fps > 0) ? 1.0f / fps : 0.0f; }

    void RequestQuit() override { m_Running = false; }
    int GetWidth() const override { return m_Width; }
    int GetHeight() const override { return m_Height; }

  private:
    void Iterate();

#ifdef CADMIUM_PLATFORM_WEB
    static Engine *s_Instance;
    static void StaticIterate();
#endif

    std::unique_ptr<Application> m_App{nullptr};
    SDL_Window *m_Window{nullptr};
    SDL_Renderer *m_Renderer{nullptr};
    SDL_Texture *m_DefaultBackground{nullptr};
    ImGuiLayer m_ImGuiLayer{};
    Timer m_Timer{};

    int m_Width{0};
    int m_Height{0};
    bool m_Running{true};
    bool m_UseDefaultBackground{true};

    struct ClearColor{ Uint8 r{0}, g{0}, b{0}, a{255}; };
    ClearColor m_ClearColor{0, 0, 0, 255};

    float m_FixedTimestep{1.0f / 60.0f};
    float m_Accumulator{0.0f};
    float m_TargetFrameTime{0.0f};
  };

} // namespace Cadmium
#endif // CADMIUM_ENGINE_HPP
