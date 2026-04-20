#ifndef CADMIUM_LAYER_HPP
#define CADMIUM_LAYER_HPP

#include <cadmium/core/engine_context.hpp>
#include <SDL3/SDL.h>
#include <functional>
#include <memory>
#include <string>

namespace Cadmium
{

  class Scene;
  class World;

  class Layer
  {
  public:
    explicit Layer(std::string name) : m_Name{std::move(name)} {}
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnFixedUpdate(float dt) {}
    virtual void OnRender(SDL_Renderer* renderer) {}
    virtual void OnEvent(SDL_Event& event) {}
    virtual void OnImGuiRender() {}

    const std::string& GetName() const { return m_Name; }

  protected:
    void Quit()                                     { m_Context->RequestQuit(); }
    int GetWidth() const                            { return m_Context->GetWidth(); }
    int GetHeight() const                           { return m_Context->GetHeight(); }
    void SetDefaultBackground(bool enabled)         { m_Context->SetDefaultBackground(enabled); }
    Scene* GetActiveScene()                         { return m_Context->GetActiveScene(); }
    void PushLayer(std::unique_ptr<Layer> layer)    { m_Context->PushLayer(std::move(layer)); }
    void PushOverlay(std::unique_ptr<Layer> layer)  { m_Context->PushOverlay(std::move(layer)); }
    void PopLayer(const std::string &name)          { m_Context->PopLayer(name); }
    void PopOverlay(const std::string &name)        { m_Context->PopOverlay(name); }
    World &GetWorld();

    void PushScene(std::unique_ptr<Scene> scene);
    void PopScene();
    void ReplaceScene(std::unique_ptr<Scene> scene);

    template <typename T>
    void Post(const T &event)
    {
      m_Context->GetEventBus().Post(event);
    }

    template <typename T>
    Cadmium::SubscriptionToken Subscribe(std::function<void(const T &)> handler)
    {
      return m_Context->GetEventBus().Subscribe<T>(std::move(handler));
    }

  private:
    friend class LayerStack;
    void SetContext(IEngineContext* context) { m_Context = context; }

    std::string m_Name;
    IEngineContext* m_Context{nullptr};
  };
} // namespace Cadmium

#endif // CADMIUM_LAYER_HPP
