#include <cadmium/core/scene.hpp>
#include <SDL3_ttf/SDL_ttf.h>

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
  void Scene::PopLayerImmediate(const std::string &name)
  {
    m_LayerStack.PopLayer(name);
  }
  void Scene::PopOverlay(const std::string &name)
  {
    m_LayerStack.RequestPopOverlay(name);
  }
  TTF_Font* Scene::GetFont()
  {
    return m_Context->GetFont();
  }
  DrawCommandQueue &Scene::GetDrawQueue()
  {
    return m_Context->GetDrawQueue();
  }
  AssetManager &Scene::GetAssets()
  {
    return m_Context->GetAssets();
  }
  sol::state &Scene::GetLua()
  {
    return m_Context->GetLua();
  }
}
// namespace Cadmium
