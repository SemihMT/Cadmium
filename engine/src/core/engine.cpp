// Engine.cpp
#include <cadmium/core/engine.hpp>
#include <stdexcept>
#include <algorithm>

namespace Cadmium
{

#ifdef CADMIUM_PLATFORM_WEB
  Engine *Engine::s_Instance = nullptr;
  void Engine::StaticIterate() { s_Instance->Iterate(); }
#endif

  Engine::Engine(std::unique_ptr<Application> app, const char *title, int width, int height)
      : m_App{std::move(app)}
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());

    m_Window = SDL_CreateWindow(title, width, height, 0);
    if (!m_Window)
      throw std::runtime_error(SDL_GetError());

    m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
    if (!m_Renderer)
      throw std::runtime_error(SDL_GetError());

    m_App->OnStart();
  }

  Engine::~Engine()
  {
    m_App->OnShutdown();
    SDL_DestroyRenderer(m_Renderer);
    SDL_DestroyWindow(m_Window);
    SDL_Quit();
  }

  void Engine::Run()
  {
#ifdef CADMIUM_PLATFORM_WEB
    s_Instance = this;
    emscripten_set_main_loop(StaticIterate, 0, 1);
#else
    while (m_App->IsRunning())
      Iterate();
#endif
  }

  void Engine::Iterate()
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_EVENT_QUIT)
        m_App->Quit();
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
    m_App->OnRender();
  }
} // namespace Cadmium
