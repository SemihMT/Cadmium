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
  /// @brief  Cadmium's brains, so to speak. Handles window creation, renderer, frame timing, scene creation etc
  class Engine : public IEngineContext
  {
  public:
    Engine(const char *title, int width, int height);
    ~Engine();

    void Run();

    void SetClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void SetVSync(bool enabled);
    void DisableDefaultBackground();

    SDL_Renderer *GetRenderer() const;
    void SetTargetFPS(int fps);

  public: // IEngineContext API
    void RequestQuit() override;
    int GetWidth() const override;
    int GetHeight() const override;
    void  SetDefaultBackground(bool enabled) override;

    Scene *GetActiveScene() override;

    void PushLayer(std::unique_ptr<Layer> layer) override;
    void PushOverlay(std::unique_ptr<Layer> layer) override;
    void PopLayer(const std::string &name) override;
    void PopOverlay(const std::string &name) override;

    EventBus &GetEventBus() override;

    void PushScene(std::unique_ptr<Scene> scene) override;
    void PopScene() override;
    void ReplaceScene(std::unique_ptr<Scene> scene) override;

  private:
    void Iterate();
    void TrySetDefaultBackground();

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

    struct ClearColor
    {
      Uint8 r{0}, g{0}, b{0}, a{255};
    };
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
