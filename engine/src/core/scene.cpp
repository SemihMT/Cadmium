#include "cadmium/ecs/components.hpp"
#include <cadmium/core/scene.hpp>
#include <SDL3_ttf/SDL_ttf.h>

namespace Cadmium
{
  // Engine-facing interface
  void Scene::Destroy()
  {
      OnDestroy();
      m_ScriptHost.Shutdown(m_World);
  }
  void Scene::SetContext(IEngineContext* context)
  {
    m_Context = context;
  }
  IEngineContext *Scene::GetContext() const { return m_Context; }
  LayerStack &Scene::GetLayerStack() { return m_LayerStack; }
  EventBus &Scene::GetEventBus()
  {
    return m_EventBus;
  }
  ScriptHost& Scene::GetScriptHost()
  {
      return m_ScriptHost;
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
  void Scene::SetDefaultBackground(bool enabled)
  {
      m_Context->SetDefaultBackground(enabled);
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

  InputManager &Scene::GetInput()
  {
    return m_Context->GetInput();
  }
  Entity Scene::CreateEntity()
  {
      auto entity = m_World.CreateEntity();
      m_World.AddComponent<Transform>(entity, {});
      return entity;
  }
  Entity Scene::CreateScriptedEntity(const std::string& scriptPath)
  {
      Entity e = CreateEntity();
      AttachScript(e, scriptPath);
      return e;
  }

  bool Scene::AttachScript(Entity e, const std::string& scriptPath)
  {
      auto loaded = GetScriptHost().LoadScript(scriptPath);
      if (!loaded.valid)
          return false;

      ScriptInstance instance{};
      instance.env = loaded.env;
      instance.name = loaded.name;
      instance.onStart = loaded.onStart;
      instance.onUpdate = loaded.onUpdate;
      instance.onDestroy = loaded.onDestroy;
      instance.env["self"] = EntityHandle{&GetWorld(), e};

      if (!GetWorld().HasComponent<Script>(e))
          GetWorld().AddComponent<Script>(e, Script{});

      GetWorld().GetComponent<Script>(e).instances.push_back(std::move(instance));
      return true;
  }
}
// namespace Cadmium
