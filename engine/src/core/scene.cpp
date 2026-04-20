#include <cadmium/core/scene.hpp>

namespace Cadmium
{
  // Engine-facing interface
  void Scene::SetContext(IEngineContext *context)
  {
    m_Context = context;
  }
  LayerStack &Scene::GetLayerStack()
  {
    return m_LayerStack;
  }
  EventBus &Scene::GetEventBus()
  {
    return m_EventBus;
  }

  void Scene::Quit()
  {
    m_Context->RequestQuit();
  }
  int Scene::GetWidth() const
  {
    return m_Context->GetWidth();
  }
  int Scene::GetHeight() const
  {
    return m_Context->GetHeight();
  }

  void Scene::PushScene(std::unique_ptr<Scene> scene)
  {
    m_Context->PushScene(std::move(scene));
  }
  void Scene::PopScene()
  {
    m_Context->PopScene();
  }
  void Scene::ReplaceScene(std::unique_ptr<Scene> scene)
  {
    m_Context->ReplaceScene(std::move(scene));
  }

  void Scene::PushLayer(std::unique_ptr<Layer> layer)
  {
    m_LayerStack.PushLayer(std::move(layer), m_Context);
  }
  void Scene::PushOverlay(std::unique_ptr<Layer> layer)
  {
    m_LayerStack.PushOverlay(std::move(layer), m_Context);
  }
  void Scene::PopLayer(const std::string &name)
  {
    m_LayerStack.RequestPopLayer(name);
  }
  void Scene::PopOverlay(const std::string &name)
  {
    m_LayerStack.RequestPopOverlay(name);
  }

} // namespace Cadmium
