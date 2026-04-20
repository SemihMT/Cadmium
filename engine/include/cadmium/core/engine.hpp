#ifndef CADMIUM_ENGINE_HPP
#define CADMIUM_ENGINE_HPP

#include <cadmium/core/scene_manager.hpp>
#include <cadmium/core/engine_context.hpp>
#include <cadmium/core/imgui_layer.hpp>
#include <cadmium/core/event_bus.hpp>
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
    Engine(const char *title, int width, int height);
    ~Engine();

    void Run();

    void SetClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void SetVSync(bool enabled) { SDL_SetRenderVSync(m_Renderer, enabled ? 1 : 0); }
    void DisableDefaultBackground();

    SDL_Renderer *GetRenderer() const { return m_Renderer; }
    void SetTargetFPS(int fps) { m_TargetFrameNS = (fps > 0)
                                                       ? static_cast<Uint64>(1e9 / fps)
                                                       : 0ULL; }

  public: // IEngineContext API
    void RequestQuit() override { m_Running = false; }
    int GetWidth() const override { return m_Width; }
    int GetHeight() const override { return m_Height; }
    Scene *GetActiveScene() override
    {
      return m_SceneManager.GetActiveScene();
    }
    void PushLayer(std::unique_ptr<Layer> layer) override
    {
      Scene *scene = m_SceneManager.GetActiveScene();
      if (!scene)
        throw std::runtime_error("PushLayer called with no active scene");
      scene->GetLayerStack().RequestPushLayer(std::move(layer));
    }
    void PushOverlay(std::unique_ptr<Layer> layer) override
    {
      Scene *scene = m_SceneManager.GetActiveScene();
      if (!scene)
        throw std::runtime_error("PushLayer called with no active scene");
      scene->GetLayerStack().RequestPushOverlay(std::move(layer));
    }
    void PopLayer(const std::string &name) override
    {
      Scene *scene = m_SceneManager.GetActiveScene();
      if (!scene)
        throw std::runtime_error("PushLayer called with no active scene");
      scene->GetLayerStack().RequestPopLayer(name);
    }
    void PopOverlay(const std::string &name) override
    {
      Scene *scene = m_SceneManager.GetActiveScene();
      if (!scene)
        throw std::runtime_error("PushLayer called with no active scene");
      scene->GetLayerStack().RequestPopOverlay(name);
    }
    EventBus &GetEventBus() override
    {
      Scene *scene = m_SceneManager.GetActiveScene();
      if (!scene)
        throw std::runtime_error("GetEventBus called with no active scene");
      return scene->GetEventBus();
    }
    void PushScene(std::unique_ptr<Scene> scene) override
    {
      m_SceneManager.RequestPush(std::move(scene));
    }
    void PopScene() override
    {
      m_SceneManager.RequestPop();
    }
    void ReplaceScene(std::unique_ptr<Scene> scene) override
    {
      m_SceneManager.RequestReplace(std::move(scene));
    }

  private:
    void Iterate();

#ifdef CADMIUM_PLATFORM_WEB
    static Engine *s_Instance;
    static void StaticIterate();
#endif

    SceneManager m_SceneManager{};
    SDL_Window *m_Window{nullptr};
    SDL_Renderer *m_Renderer{nullptr};
    SDL_Texture *m_DefaultBackground{nullptr};
    ImGuiLayer m_ImGuiLayer{};

    int m_Width{0};
    int m_Height{0};
    bool m_Running{true};
    bool m_UseDefaultBackground{true};

    struct ClearColor{ Uint8 r{0}, g{0}, b{0}, a{255}; };
    ClearColor m_ClearColor{0, 0, 0, 255};

    float m_FixedTimestep{1.0f / 60.0f};
    float m_Accumulator{0.0f};
    float m_MaxDeltaTime{0.25f};
    Uint64 m_TargetFrameNS{0};
    Uint64 m_LastCounter{0};
    Uint64 m_Frequency{0};

  };

} // namespace Cadmium
#endif // CADMIUM_ENGINE_HPP
