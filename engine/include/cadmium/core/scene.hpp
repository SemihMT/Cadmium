#ifndef CADMIUM_SCENE_HPP
#define CADMIUM_SCENE_HPP

#include "cadmium/ecs/components.hpp"
#include <cadmium/core/layer_stack.hpp>
#include <cadmium/core/event_bus.hpp>
#include <cadmium/ecs/world.hpp>
#include <cadmium/scripting/script_host.hpp>
#include <string>
#include <memory>
#include <functional>

namespace Cadmium
{
  class IEngineContext;
  class Scene
  {
  public:
    explicit Scene(std::string name) : m_Name{std::move(name)} { m_World.SetOwningScene(this); }

    virtual ~Scene() = default;
    void Enter()
    {
        m_ScriptHost.Configure(m_Context->GetInput(), m_World);
        OnEnter();
    }
    void Destroy();

    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnDestroy() {}

    const std::string &GetName() const { return m_Name; }
    World& GetWorld() { return m_World; }

  public: // Engine-facing interface
    void SetContext(IEngineContext *context);
    IEngineContext *GetContext() const;
    LayerStack &GetLayerStack();
    EventBus &GetEventBus();
    ScriptHost& GetScriptHost();

  protected:
    void Quit();
    int GetWidth() const;
    int GetHeight() const;
    void SetDefaultBackground(bool enabled);
    void PushScene(std::unique_ptr<Scene> scene);
    void PopScene();
    void ReplaceScene(std::unique_ptr<Scene> scene);
    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> layer);
    void PopLayer(const std::string &name);
    void PopLayerImmediate(const std::string& name); // Skips the deferred pop request. Only for internal engine use.
    void PopOverlay(const std::string &name);
    TTF_Font* GetFont();
    DrawCommandQueue &GetDrawQueue();
    AssetManager& GetAssets();
    InputManager& GetInput();

    template <typename T>
    void Post(const T &event)
    {
      m_EventBus.Post(event);
    }

    template <typename T>
    SubscriptionToken Subscribe(std::function<void(const T &)> handler)
    {
      return m_EventBus.Subscribe<T>(std::move(handler));
    }

    Entity CreateEntity();
    Entity CreateScriptedEntity(const std::string& scriptPath);
    void DestroyEntity(Entity e) { m_World.DestroyEntity(e); }

    bool AttachScript(Entity e, const std::string& scriptPath);

    template <typename T, typename... Args>
    T &RegisterSystem(int order, Args &&...args)
    {
      return m_World.RegisterSystem<T>(order, std::forward<Args>(args)...);
    }

  private:
    std::string m_Name;
    EventBus m_EventBus;
    LayerStack m_LayerStack;
    IEngineContext *m_Context{nullptr};
    ScriptHost m_ScriptHost;
    World m_World;
  };

} // namespace Cadmium

#endif // CADMIUM_SCENE_HPP
