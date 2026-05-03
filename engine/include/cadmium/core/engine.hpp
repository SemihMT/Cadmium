#ifndef CADMIUM_ENGINE_HPP
#define CADMIUM_ENGINE_HPP

#include <cadmium/core/draw_command_queue.hpp>
#include <cadmium/scripting/lua_bindings.hpp>
#include <cadmium/core/engine_context.hpp>
#include <cadmium/core/scene_manager.hpp>
#include <cadmium/core/input_manager.hpp>
#include <cadmium/core/imgui_layer.hpp>
#include <cadmium/core/event_bus.hpp>

#include <SDL3/SDL.h>
#include <sol/sol.hpp>

#include <memory>

#ifdef CADMIUM_PLATFORM_WEB
#include <emscripten.h>
#endif
#include <SDL3_ttf/SDL_ttf.h>


namespace Cadmium
{
  class Engine : public IEngineContext
  {
  public:
    Engine(const char *title, int width, int height);
    ~Engine();

    void Run();

    void SetClearColor(float r, float g, float b, float a = 1.0f);
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

    TTF_Font* GetFont() override;
    DrawCommandQueue& GetDrawQueue() override;
    sol::state& GetLua() override;

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
    TTF_Font *m_Font{nullptr};
    SDL_Texture *m_DefaultBackground{nullptr};
    ImGuiLayer m_ImGuiLayer{};
    InputManager m_Input{};
    DrawCommandQueue m_DrawQueue{};
    Lua::SceneBindingState m_SceneState{};
    sol::state m_Lua{};

    int m_Width{0};
    int m_Height{0};
    bool m_Running{true};
    bool m_UseDefaultBackground{true};

    Cadmium::Color m_ClearColor{0.0f, 0.0f, 0.0f, 1.0f};

    float m_FixedTimestep{1.0f / 60.0f};
    float m_Accumulator{0.0f};
    float m_MaxDeltaTime{0.25f};
    Uint64 m_TargetFrameNS{0};
    Uint64 m_LastCounter{0};
    Uint64 m_Frequency{0};
  };

} // namespace Cadmium
#endif // CADMIUM_ENGINE_HPP
