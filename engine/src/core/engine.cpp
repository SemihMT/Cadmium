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

#ifdef CADMIUM_IMGUI
  void Engine::InitImGui()
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(m_Window, m_Renderer);
    ImGui_ImplSDLRenderer3_Init(m_Renderer);
  }

  void Engine::ShutdownImGui()
  {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }

  void Engine::BeginImGuiFrame()
  {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
  }

  void Engine::EndImGuiFrame()
  {
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer);
  }
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

#ifdef CADMIUM_PLATFORM_WEB
    SetVSync(true);
#endif

    // Load default background
    std::string bgPath = AssetPath("assets/Cadmium-bg.bmp");
    SDL_Surface *surface = SDL_LoadBMP(bgPath.c_str());
    if (surface)
    {
      m_DefaultBackground = SDL_CreateTextureFromSurface(m_Renderer, surface);
      SDL_DestroySurface(surface);
    }
#ifdef CADMIUM_IMGUI
    InitImGui();
#endif
    m_App->OnStart();
  }

  Engine::~Engine()
  {
    m_App->OnShutdown();
#ifdef CADMIUM_IMGUI
    ShutdownImGui();
#endif
    if (m_DefaultBackground)
      SDL_DestroyTexture(m_DefaultBackground);
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

    if (m_TargetFrameTime > 0.0f)
    {
      float elapsed = m_Timer.Peek();
      if (elapsed < m_TargetFrameTime)
      {
        SDL_DelayNS(static_cast<Uint64>((m_TargetFrameTime - elapsed) * 1e9f));
      }
    }
    float dt = m_Timer.DeltaTimeClamped();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
#ifdef CADMIUM_IMGUI
      ImGui_ImplSDL3_ProcessEvent(&event);
#endif
      if (event.type == SDL_EVENT_QUIT)
        m_App->Quit();
      m_App->OnEvent(event);
    }

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
      int w, h;
      SDL_GetWindowSize(m_Window, &w, &h);
      SDL_FRect dst{0, 0, static_cast<float>(w), static_cast<float>(h)};
      SDL_RenderTexture(m_Renderer, m_DefaultBackground, nullptr, &dst);
    }

    m_App->OnRender(m_Renderer);
#ifdef CADMIUM_IMGUI
    BeginImGuiFrame();
    m_App->OnImGuiRender();
    EndImGuiFrame();
#endif
    SDL_RenderPresent(m_Renderer);
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
