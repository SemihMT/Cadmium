#ifndef CADMIUM_SCENE_HPP
#define CADMIUM_SCENE_HPP

#include <cadmium/core/layer_stack.hpp>
#include <cadmium/core/event_bus.hpp>
#include <cadmium/ecs/world.hpp>
#include <string>
#include <memory>
#include <functional>

namespace Cadmium
{
  class IEngineContext;
  class Scene
  {
  public:
    explicit Scene(std::string name)
        : m_Name{std::move(name)} {}

    virtual ~Scene() = default;

    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnDestroy() {}

    const std::string &GetName() const { return m_Name; }
    World& GetWorld() { return m_World; }

  public: // Engine-facing interface
    void SetContext(IEngineContext *context);
    LayerStack &GetLayerStack();
    EventBus &GetEventBus();

  protected:
    void Quit();
    int GetWidth() const;
    int GetHeight() const;
    void PushScene(std::unique_ptr<Scene> scene);
    void PopScene();
    void ReplaceScene(std::unique_ptr<Scene> scene);
    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> layer);
    void PopLayer(const std::string &name);
    void PopOverlay(const std::string &name);

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

    Entity CreateEntity() { return m_World.CreateEntity(); }
    void DestroyEntity(Entity e) { m_World.DestroyEntity(e); }

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
    World m_World;
  };

} // namespace Cadmium

#endif // CADMIUM_SCENE_HPP
