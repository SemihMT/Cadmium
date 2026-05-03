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

    m_AssetManager.Init(m_Renderer);

    if(!TTF_Init())
      throw std::runtime_error(SDL_GetError());

    m_Font = TTF_OpenFont(AssetPath("assets/fonts/JetBrainsMono-Regular.ttf").c_str(), 96);
    if (!m_Font)
      SDL_Log("Failed to load font: %s", SDL_GetError());

    m_Frequency = SDL_GetPerformanceFrequency();
    m_LastCounter = SDL_GetPerformanceCounter();

#ifdef CADMIUM_PLATFORM_WEB
    SetVSync(true);
#endif

    TrySetDefaultBackground();

    m_ImGuiLayer.Init(m_Window, m_Renderer);

    m_Lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string,
        sol::lib::io,     // for file loading - restrict in sandbox build
        sol::lib::os,     // TODO: Whitelist functionality to make os library access safe
        sol::lib::package // for require()
    );
    Lua::BindPhase1(m_Lua, m_Input, m_DrawQueue, m_SceneState);
  }

  Engine::~Engine()
  {
    m_ImGuiLayer.Shutdown();
    if (m_Font)
      TTF_CloseFont(m_Font);
    TTF_Quit();
    SDL_DestroyRenderer(m_Renderer);
    SDL_DestroyWindow(m_Window);
    SDL_Quit();
  }

  void Engine::Run()
  {
    // Process any scenes pushed after engine init
    // but before Run() was called
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

    m_Input.BeginFrame();

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
      {
        RequestQuit();
      }
      if (event.type == SDL_EVENT_MOUSE_WHEEL)
      {
        m_Input.OnMouseWheel(event.wheel.x, event.wheel.y);
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED)
      {
        m_Width = event.window.data1;
        m_Height = event.window.data2;
      }

      for (auto it = layerStack.rbegin(); it != layerStack.rend(); ++it)
        (*it)->OnEvent(event);
    }
    m_Input.SnapshotPost();
    m_SceneState.Width = static_cast<float>(m_Width);
    m_SceneState.Height = static_cast<float>(m_Height);
    m_SceneState.Time += dt;
    m_SceneState.DeltaTime = dt;
    Lua::UpdateSceneBindings(m_Lua, m_SceneState);

    m_Accumulator += dt;
    while (m_Accumulator >= m_FixedTimestep)
    {
      for (auto &layer : layerStack)
        layer->OnFixedUpdate(m_FixedTimestep);
      scene->GetWorld().Update(m_FixedTimestep);
      m_Accumulator -= m_FixedTimestep;
    }

    for (auto &layer : layerStack)
      layer->OnUpdate(dt);

    auto toUint8 = [](float channel)
    {
      channel = std::clamp(channel, 0.0f, 1.0f);
      return static_cast<Uint8>(channel * 255.0f);
    };

    SDL_SetRenderDrawColor(m_Renderer,
                           toUint8(m_ClearColor.r),
                           toUint8(m_ClearColor.g),
                           toUint8(m_ClearColor.b),
                           toUint8(m_ClearColor.a));
    SDL_RenderClear(m_Renderer);

    if (m_UseDefaultBackground)
    {
      SDL_Texture *bg = m_AssetManager.GetTexture(m_DefaultBackgroundHandle);
      if (bg)
        SDL_RenderTexture(m_Renderer, bg, nullptr, nullptr);
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
  void Engine::TrySetDefaultBackground()
  {
    std::string bgPath = AssetPath("assets/Cadmium-bg.bmp");
    m_DefaultBackgroundHandle = m_AssetManager.LoadTexture(bgPath);

    if (m_DefaultBackgroundHandle == k_InvalidHandle)
      SDL_Log("Cadmium: default background not found at '%s'", bgPath.c_str());
  }

  void Engine::SetClearColor(float r, float g, float b, float a)
  {
    m_ClearColor = {r, g, b, a};
  }

  void Engine::SetClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
  {
    SetClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
  }

  void Engine::SetVSync(bool enabled)
  {
    SDL_SetRenderVSync(m_Renderer, enabled ? 1 : 0);
  }
  void Engine::DisableDefaultBackground()
  {
    m_UseDefaultBackground = false;
  }
  SDL_Renderer *Engine::GetRenderer() const
  {
    return m_Renderer;
  }
  void Engine::SetTargetFPS(int fps)
  {
    m_TargetFrameNS = (fps > 0)
                          ? static_cast<Uint64>(1e9 / fps)
                          : 0ULL;
  }
  // IEngineContext API:
  void Engine::RequestQuit()
  {
    m_Running = false;
  }

  int Engine::GetWidth() const
  {
    return m_Width;
  }

  int Engine::GetHeight() const
  {
    return m_Height;
  }

  void Engine::SetDefaultBackground(bool enabled)
  {
    m_UseDefaultBackground = enabled;
  }

  Scene *Engine::GetActiveScene()
  {
    return m_SceneManager.GetActiveScene();
  }

  void Engine::PushLayer(std::unique_ptr<Layer> layer)
  {
    Scene *scene = m_SceneManager.GetActiveScene();
    if (!scene)
      throw std::runtime_error("PushLayer called with no active scene");
    scene->GetLayerStack().RequestPushLayer(std::move(layer));
  }

  void Engine::PushOverlay(std::unique_ptr<Layer> layer)
  {
    Scene *scene = m_SceneManager.GetActiveScene();
    if (!scene)
      throw std::runtime_error("PushOverlay called with no active scene");
    scene->GetLayerStack().RequestPushOverlay(std::move(layer));
  }

  void Engine::PopLayer(const std::string &name)
  {
    Scene *scene = m_SceneManager.GetActiveScene();
    if (!scene)
      throw std::runtime_error("PopLayer called with no active scene");
    scene->GetLayerStack().RequestPopLayer(name);
  }

  void Engine::PopOverlay(const std::string &name)
  {
    Scene *scene = m_SceneManager.GetActiveScene();
    if (!scene)
      throw std::runtime_error("PopOverlay called with no active scene");
    scene->GetLayerStack().RequestPopOverlay(name);
  }

  EventBus &Engine::GetEventBus()
  {
    Scene *scene = m_SceneManager.GetActiveScene();
    if (!scene)
      throw std::runtime_error("GetEventBus called with no active scene");
    return scene->GetEventBus();
  }

  void Engine::PushScene(std::unique_ptr<Scene> scene)
  {
    m_SceneManager.RequestPush(std::move(scene));
  }

  void Engine::PopScene()
  {
    m_SceneManager.RequestPop();
  }

  void Engine::ReplaceScene(std::unique_ptr<Scene> scene)
  {
    m_SceneManager.RequestReplace(std::move(scene));
  }

  TTF_Font* Engine::GetFont()
  {
    return m_Font;
  }

  DrawCommandQueue &Engine::GetDrawQueue()
  {
    return m_DrawQueue;
  }

  AssetManager &Engine::GetAssets()
  {
    return m_AssetManager;
  }
  sol::state &Engine::GetLua()
  {
    return m_Lua;
  }
} // namespace Cadmium
