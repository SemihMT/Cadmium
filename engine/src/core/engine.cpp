#include <cadmium/core/layer.hpp>
#include <cadmium/render/renderer_backend.hpp>
#include <algorithm>
#include <cadmium/core/assets.hpp>
#include <cadmium/core/engine.hpp>
#include <cadmium/core/logger.hpp>
#include <cadmium/render/sdl_renderer.hpp>
#include <cadmium/render/webgpu_renderer.hpp>
#include <memory>
#include <stdexcept>


namespace Cadmium
{
#ifdef CADMIUM_PLATFORM_WEB
    Engine* Engine::s_Instance = nullptr;
    void Engine::StaticIterate()
    {
        s_Instance->Iterate();
    }
#endif

    Engine::Engine(const char* title, int width, int height, RendererBackend backend) : m_Width{width}, m_Height{height}, m_Backend{backend}
    {
        Cadmium::AddStdoutSink();
        Cadmium::Log::Info("Engine", "Initializing engine!");
        m_AssetManager.SetProjectRoot(AssetPath("assets/"));

        if (!SDL_Init(SDL_INIT_VIDEO))
            throw std::runtime_error(SDL_GetError());

        m_Window = SDL_CreateWindow(title, width, height, 0);
        if (!m_Window)
            throw std::runtime_error(SDL_GetError());

        if (m_Backend == RendererBackend::SDL2D)
        {
            m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
            if (!m_Renderer)
                throw std::runtime_error(SDL_GetError());

            if (!TTF_Init())
                throw std::runtime_error(SDL_GetError());

            m_RenderBackend = std::make_unique<SDLRenderer>(m_Renderer, m_TextCache);
            m_TextCache.Init(*m_RenderBackend, AssetPath("assets/fonts/JetBrainsMono-Regular.ttf"), 96.0f);
            m_AssetManager.Init(*m_RenderBackend);
            m_ImGuiLayer.InitForSDLRenderer(m_Window, m_Renderer, *m_RenderBackend);
            m_RendererReady = true;
        }
        else
        {
            auto webgpu = std::make_unique<WebGPURenderer>(m_Window, m_Width, m_Height, m_TextCache);
            webgpu->RequestDevice(
                [this](bool ok)
                {
                    m_RendererReady = ok;
                    if (!ok)
                    {
                        Cadmium::Log::Error("Engine", "WebGPU device negotiation failed");
                        return;
                    }
                    m_AssetManager.Init(*m_RenderBackend);

                    WebGPURenderer& gpu = static_cast<WebGPURenderer&>(*m_RenderBackend);
                    gpu.CreateFlatColorPipeline();
                    gpu.CreateTexturedPipeline();

                    if (!TTF_Init())
                    {
                        Cadmium::Log::Error("Engine", "TTF_Init failed: {}", SDL_GetError());
                        m_RendererReady = false;
                        return;
                    }

                    m_TextCache.Init(*m_RenderBackend,
                                     AssetPath("assets/fonts/JetBrainsMono-Regular.ttf"),
                                     96.0f);

                    m_ImGuiLayer.InitForWebGPU(m_Window, gpu.GetDevice(), gpu.GetSurfaceFormat(), *m_RenderBackend);
                    if (m_PendingViewportEnable)
                    {
                        m_PendingViewportEnable = false;
                        EnableViewport(m_PendingViewportW,m_PendingViewportH);
                    }
                });
            m_RenderBackend = std::move(webgpu);
        }

        m_Frequency = SDL_GetPerformanceFrequency();
        m_LastCounter = SDL_GetPerformanceCounter();

#ifdef CADMIUM_PLATFORM_WEB
        SetVSync(true);
#endif

        //TrySetDefaultBackground();

    }

    Engine::~Engine()
    {
        m_ImGuiLayer.Shutdown();

        if (m_Backend == RendererBackend::SDL2D)
        {
            SDL_DestroyRenderer(m_Renderer);
            TTF_Quit();
        }

        SDL_DestroyWindow(m_Window);
        SDL_Quit();
    }

    void Engine::Run()
    {
        // Process any scenes pushed after engine init
        // but before Run() was called
        m_SceneManager.FlushPending(this);
        m_Started = true;
#ifdef CADMIUM_PLATFORM_WEB
        s_Instance = this;
        emscripten_set_main_loop(StaticIterate, 0, 1);
        // unreachable on web, emscripten_set_main_loop does not return in the traditional sense
        // Because of that, app shutdown is also handled in iterate()
#else
        while (m_Running)
            Iterate();

        if (auto* scene = m_SceneManager.GetActiveScene())
            scene->GetLayerStack().Clear();
        m_GlobalLayers.Clear();
#endif
    }

    void Engine::Iterate()
    {
        if (!m_RendererReady)
            return;
        Uint64 frameStart = SDL_GetPerformanceCounter();

        Uint64 counter = frameStart;
        float dt = (counter - m_LastCounter) / (float)m_Frequency;
        m_LastCounter = counter;

        dt = std::min(dt, m_MaxDeltaTime);

        m_Input.BeginFrame();

        Scene* scene = m_SceneManager.GetActiveScene();
        if (!scene)
            return;

        auto& layerStack = scene->GetLayerStack();
        auto& eventBus = scene->GetEventBus();

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
                m_RenderBackend->Resize(m_Width, m_Height);
            }

            for (auto it = m_GlobalLayers.rbegin(); it != m_GlobalLayers.rend(); ++it)
                (*it)->OnEvent(event);

            for (auto it = layerStack.rbegin(); it != layerStack.rend(); ++it)
                (*it)->OnEvent(event);
        }
        m_Input.SnapshotPost();
        if(m_UseViewport && m_Viewport.IsReady())
        {
          float rawX = m_Input.MouseX();
          float rawY = m_Input.MouseY();
          m_Input.SetMousePosition(rawX - m_Viewport.GetScreenX(),rawY - m_Viewport.GetScreenY());
        }

        // TODO: expose accumulator to user code as the alpha value between frames for interpolation
        m_Accumulator += dt;
        while (m_Accumulator >= m_FixedTimestep)
        {
          for (auto &layer : m_GlobalLayers)
              layer->OnFixedUpdate(m_FixedTimestep);

          for (auto& layer : layerStack)
              layer->OnFixedUpdate(m_FixedTimestep);

          // Gated by m_SimulationPaused (see SetSimulationPaused) so the
          // editor's Edit/Play toggle can actually stop gameplay logic
          // (ScriptSystem::OnUpdate -> OnStart/OnUpdate hooks) from running
          // only the ECS/script simulation step is skipped.
          if (!m_SimulationPaused)
              scene->GetWorld().Update(m_FixedTimestep);
          m_Accumulator -= m_FixedTimestep;
        }

        for (auto &layer : m_GlobalLayers)
            layer->OnUpdate(dt);

        for (auto& layer : layerStack)
            layer->OnUpdate(dt);

        if (m_UseViewport && m_Viewport.IsReady())
            m_Viewport.Bind();

        m_RenderBackend->BeginFrame(m_ClearColor);

       if (m_UseDefaultBackground)
           m_RenderBackend->DrawFullscreenTexture(m_DefaultBackgroundHandle);

       for (auto& layer : layerStack)
           layer->OnRender(m_Renderer);
       for (auto& layer : m_GlobalLayers)
           layer->OnRender(m_Renderer);

        if (m_UseViewport && m_Viewport.IsReady())
           m_Viewport.Unbind();

       for (auto& layer : m_GlobalLayers)
           layer->OnImGuiRender();
       for (auto& layer : layerStack)
           layer->OnImGuiRender();

       m_RenderBackend->EndFrame();



       eventBus.Dispatch();
       layerStack.FlushPending(this);
       m_GlobalLayers.FlushPending(this);
       m_SceneManager.FlushPending(this);

       if (m_TargetFrameNS > 0)
       {
           Uint64 now = SDL_GetPerformanceCounter();
           Uint64 elapsed = now - frameStart;

           Uint64 targetTicks = (m_TargetFrameNS * m_Frequency) / 1'000'000'000ULL;

           if (elapsed < targetTicks)
           {
               Uint64 remaining = targetTicks - elapsed;

               Uint32 delayMS = static_cast<Uint32>((remaining * 1000) / m_Frequency);

               if (delayMS > 0)
                   SDL_Delay(delayMS);
           }
        }
#ifdef CADMIUM_PLATFORM_WEB
        if (!m_Running)
        {
            if (auto* scene = m_SceneManager.GetActiveScene())
                scene->GetLayerStack().Clear();
            m_GlobalLayers.Clear();
            emscripten_cancel_main_loop();
        }
#endif
    }
    void Engine::TrySetDefaultBackground()
    {
        m_DefaultBackgroundHandle = m_AssetManager.LoadTexture("Cadmium-bg.bmp");
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
    void Engine::EnableViewport(int w, int h)
    {
        if (m_Backend == RendererBackend::WebGPU && !m_RendererReady)
        {
            m_PendingViewportEnable = true;
            m_PendingViewportW = w;
            m_PendingViewportH = h;
            return;
        }

        bool ok = (m_Backend == RendererBackend::SDL2D)
            ? m_Viewport.InitSDL(m_Renderer, w, h)
            : m_Viewport.InitWebGPU(static_cast<WebGPURenderer&>(*m_RenderBackend), w, h);
        m_UseViewport = ok;
    }
    void Engine::ResizeViewport(int w, int h)
    {
        m_UseViewport = m_Viewport.Resize(w, h);
    }
    void Engine::DisableViewport()
    {
        m_PendingViewportEnable = false;
        if (m_Backend == RendererBackend::WebGPU)
            static_cast<WebGPURenderer&>(*m_RenderBackend).ClearViewportRenderTarget();
        m_UseViewport = false;
    }
    SDL_Texture* Engine::GetRenderTarget()
    {
        return m_UseViewport ? m_Viewport.GetSDLTexture() : nullptr;
    }
    Editor::RenderViewport& Engine::GetViewport()
    {
        return m_Viewport;
    }
    SDL_Renderer* Engine::GetNativeRenderer() const
    {
        return m_Renderer;
    }
    void Engine::SetTargetFPS(int fps)
    {
        m_TargetFrameNS = (fps > 0) ? static_cast<Uint64>(1e9 / fps) : 0ULL;
    }
    void Engine::PushGlobalOverlay(std::unique_ptr<Cadmium::Layer> layer)
    {
        m_GlobalLayers.PushOverlay(std::move(layer), this);
    }
    // IEngineContext API:
    void Engine::RequestQuit()
    {
        m_Running = false;
    }

    void Engine::SetSimulationPaused(bool paused)
    {
        m_SimulationPaused = paused;
    }

    bool Engine::IsSimulationPaused() const
    {
        return m_SimulationPaused;
    }

    int Engine::GetWidth() const
    {
        return (m_UseViewport && m_Viewport.IsReady()) ? m_Viewport.GetWidth() : m_Width;
    }

    int Engine::GetHeight() const
    {
        return (m_UseViewport && m_Viewport.IsReady()) ? m_Viewport.GetHeight() : m_Height;
    }
    IRenderer& Engine::GetRenderer()
    {
        return *m_RenderBackend;
    }

    void Engine::SetDefaultBackground(bool enabled)
    {
        m_UseDefaultBackground = enabled;
    }

    Scene* Engine::GetActiveScene()
    {
        return m_SceneManager.GetActiveScene();
    }

    void Engine::PushLayer(std::unique_ptr<Layer> layer)
    {
        if (!m_Started)
            m_SceneManager.FlushPending(this);

        Scene* scene = m_SceneManager.GetActiveScene();
        if (!scene)
            throw std::runtime_error("PushLayer called with no active scene");
        scene->GetLayerStack().RequestPushLayer(std::move(layer));
    }

    void Engine::PushOverlay(std::unique_ptr<Layer> layer)
    {
        if (!m_Started)
            m_SceneManager.FlushPending(this);

        Scene* scene = m_SceneManager.GetActiveScene();
        if (!scene)
            throw std::runtime_error("PushOverlay called with no active scene");
        scene->GetLayerStack().RequestPushOverlay(std::move(layer));
    }

    void Engine::PopLayer(const std::string& name)
    {
        if (!m_Started)
            m_SceneManager.FlushPending(this);

        Scene* scene = m_SceneManager.GetActiveScene();
        if (!scene)
            throw std::runtime_error("PopLayer called with no active scene");
        scene->GetLayerStack().RequestPopLayer(name);
    }

    void Engine::PopOverlay(const std::string& name)
    {
        if (!m_Started)
            m_SceneManager.FlushPending(this);

        Scene* scene = m_SceneManager.GetActiveScene();
        if (!scene)
            throw std::runtime_error("PopOverlay called with no active scene");
        scene->GetLayerStack().RequestPopOverlay(name);
    }

    EventBus& Engine::GetEventBus()
    {
        if (!m_Started)
            m_SceneManager.FlushPending(this);

        Scene* scene = m_SceneManager.GetActiveScene();
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

    TextCache& Engine::GetTextCache()
    {
        return m_TextCache;
    }

    DrawCommandQueue& Engine::GetDrawQueue()
    {
        return m_DrawQueue;
    }

    AssetManager& Engine::GetAssets()
    {
        return m_AssetManager;
    }
    InputManager& Engine::GetInput()
    {
        return m_Input;
    }
} // namespace Cadmium
