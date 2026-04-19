// Engine.cpp
#include <cadmium/core/engine.hpp>
#include <cadmium/core/assets.hpp>
#include <stdexcept>
#include <algorithm>

namespace Cadmium
{
#ifdef CADMIUM_PLATFORM_WEB
  Engine *Engine::s_Instance = nullptr;
  void Engine::StaticIterate() { s_Instance->Iterate(); }
#endif

  Engine::Engine(std::unique_ptr<Application> app, const char *title, int width, int height)
      : m_App{std::move(app)}, m_Width{width}, m_Height{height}
  {

    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());

    m_Window = SDL_CreateWindow(title, width, height, 0);
    if (!m_Window)
      throw std::runtime_error(SDL_GetError());

    m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
    if (!m_Renderer)
      throw std::runtime_error(SDL_GetError());

#ifdef CADMIUM_PLATFORM_WEB
    SetVSync(true);
#endif

    std::string bgPath = AssetPath("assets/Cadmium-bg.bmp");
    SDL_Surface *surface = SDL_LoadBMP(bgPath.c_str());
    if (surface)
    {
      m_DefaultBackground = SDL_CreateTextureFromSurface(m_Renderer, surface);
      SDL_DestroySurface(surface);
    }
    else
    {
      SDL_Log("Cadmium: default background not found at '%s'", bgPath.c_str());
    }

    m_ImGuiLayer.Init(m_Window, m_Renderer);
    m_App->SetContext(this);
  }

  Engine::~Engine()
  {
    m_ImGuiLayer.Shutdown();
    if (m_DefaultBackground)
      SDL_DestroyTexture(m_DefaultBackground);
    SDL_DestroyRenderer(m_Renderer);
    SDL_DestroyWindow(m_Window);
    SDL_Quit();
  }

  void Engine::Run()
  {
    m_App->OnStart();
#ifdef CADMIUM_PLATFORM_WEB
    s_Instance = this;
    emscripten_set_main_loop(StaticIterate, 0, 1);
    // unreachable on web, emscripten_set_main_loop does not return in the traditional sense
    // Because of that, app shutdown is also handled in iterate()
#else
    while (m_Running)
      Iterate();

    m_App->OnShutdown();
#endif
  }



  void Engine::Iterate()
  {

    if (m_TargetFrameTime > 0.0f)
    {
      float elapsed = m_Timer.Peek();
      if (elapsed < m_TargetFrameTime)
      {
        SDL_DelayNS(static_cast<Uint64>((m_TargetFrameTime - elapsed) * 1e9f));
      }
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      m_ImGuiLayer.ProcessEvent(event);
      if (event.type == SDL_EVENT_QUIT)
      m_Running = false;

      if (event.type == SDL_EVENT_WINDOW_RESIZED)
      {
        m_Width = event.window.data1;
        m_Height = event.window.data2;
      }

      m_App->OnEvent(event);
    }

    float dt = m_Timer.DeltaTimeClamped();
    m_Accumulator += dt;
    while (m_Accumulator >= m_FixedTimestep)
    {
      m_App->OnFixedUpdate(m_FixedTimestep);
      m_Accumulator -= m_FixedTimestep;
    }

    m_App->OnUpdate(dt);

    SDL_SetRenderDrawColor(m_Renderer,
                           m_ClearColor.r,
                           m_ClearColor.g,
                           m_ClearColor.b,
                           m_ClearColor.a);
    SDL_RenderClear(m_Renderer);

    if (m_UseDefaultBackground && m_DefaultBackground)
    {
      SDL_FRect dst{0, 0, static_cast<float>(m_Width), static_cast<float>(m_Height)};
      SDL_RenderTexture(m_Renderer, m_DefaultBackground, nullptr, &dst);
    }

    m_App->OnRender(m_Renderer);

    m_ImGuiLayer.Begin();
    m_App->OnImGuiRender();
    m_ImGuiLayer.End(m_Renderer);
    SDL_RenderPresent(m_Renderer);

#ifdef CADMIUM_PLATFORM_WEB
    if (!m_Running)
    {
      m_App->OnShutdown();
      emscripten_cancel_main_loop();
    }
#endif
  }
  void Engine::SetClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
  {
    m_ClearColor = {r, g, b, a};
  }
  void Engine::DisableDefaultBackground()
  {
    m_UseDefaultBackground = false;
  }
} // namespace Cadmium
