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

  Engine::Engine(const char *title, int width, int height)
      : m_Width{width}, m_Height{height}
  {

    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());

    m_Window = SDL_CreateWindow(title, width, height, 0);
    if (!m_Window)
      throw std::runtime_error(SDL_GetError());

    m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
    if (!m_Renderer)
      throw std::runtime_error(SDL_GetError());

    m_Frequency = SDL_GetPerformanceFrequency();
    m_LastCounter = SDL_GetPerformanceCounter();
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
    m_SceneManager.FlushPending(this);
#ifdef CADMIUM_PLATFORM_WEB
    s_Instance = this;
    emscripten_set_main_loop(StaticIterate, 0, 1);
    // unreachable on web, emscripten_set_main_loop does not return in the traditional sense
    // Because of that, app shutdown is also handled in iterate()
#else
    while (m_Running)
      Iterate();

    if (auto *scene = m_SceneManager.GetActiveScene())
      scene->GetLayerStack().Clear();
#endif
  }

  void Engine::Iterate()
  {
    Uint64 frameStart = SDL_GetPerformanceCounter();

    Uint64 counter = frameStart;
    float dt = (counter - m_LastCounter) / (float)m_Frequency;
    m_LastCounter = counter;

    dt = std::min(dt, m_MaxDeltaTime);

    Scene *scene = m_SceneManager.GetActiveScene();
    if (!scene)
      return;

    auto &layerStack = scene->GetLayerStack();
    auto &eventBus = scene->GetEventBus();

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

      for (auto it = layerStack.rbegin(); it != layerStack.rend(); ++it)
        (*it)->OnEvent(event);
    }

    m_Accumulator += dt;
    while (m_Accumulator >= m_FixedTimestep)
    {
      for (auto &layer : layerStack)
        layer->OnFixedUpdate(m_FixedTimestep);
      m_Accumulator -= m_FixedTimestep;
    }

    scene->GetWorld().Update(m_FixedTimestep);

    for (auto &layer : layerStack)
      layer->OnUpdate(dt);

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

    for (auto &layer : layerStack)
      layer->OnRender(m_Renderer);

    m_ImGuiLayer.Begin();
    for (auto &layer : layerStack)
      layer->OnImGuiRender();
    m_ImGuiLayer.End(m_Renderer);

    SDL_RenderPresent(m_Renderer);
    eventBus.Dispatch();
    layerStack.FlushPending(this);
    m_SceneManager.FlushPending(this);

    if (m_TargetFrameNS > 0)
    {
      Uint64 now = SDL_GetPerformanceCounter();
      Uint64 elapsed = now - frameStart;

      Uint64 targetTicks =
          (m_TargetFrameNS * m_Frequency) / 1'000'000'000ULL;

      if (elapsed < targetTicks)
      {
        Uint64 remaining = targetTicks - elapsed;

        Uint32 delayMS =
            static_cast<Uint32>((remaining * 1000) / m_Frequency);

        if (delayMS > 0)
          SDL_Delay(delayMS);

        while ((SDL_GetPerformanceCounter() - frameStart) < targetTicks)
        {
          // busy wait for sub-millisecond precision
        }
      }
    }
#ifdef CADMIUM_PLATFORM_WEB
    if (!m_Running)
    {
      if (auto *scene = m_SceneManager.GetActiveScene())
        scene->GetLayerStack().Clear();
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
