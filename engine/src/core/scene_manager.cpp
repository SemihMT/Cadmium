#include <cadmium/core/scene_manager.hpp>

namespace Cadmium
{
  Scene* SceneManager::GetActiveScene()
  {
    if (m_Stack.empty()) return nullptr;
    return m_Stack.back().get();
  }

  void SceneManager::RequestPush(std::unique_ptr<Scene> scene)
  {
    m_Pending.push_back(PushCmd{ std::move(scene) });
  }

  void SceneManager::RequestPop()
  {
    m_Pending.push_back(PopCmd{});
  }

  void SceneManager::RequestReplace(std::unique_ptr<Scene> scene)
  {
    m_Pending.push_back(ReplaceCmd{ std::move(scene) });
  }

  void SceneManager::FlushPending(IEngineContext* context)
  {
    if (m_Pending.empty()) return;

    std::vector<Command> toProcess;
    std::swap(toProcess, m_Pending);

    for (auto& cmd : toProcess)
    {
      std::visit([&](auto&& c)
      {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, PushCmd>)
          ApplyPush(std::move(c.scene), context);
        else if constexpr (std::is_same_v<T, PopCmd>)
          ApplyPop(context);
        else if constexpr (std::is_same_v<T, ReplaceCmd>)
          ApplyReplace(std::move(c.scene), context);
      }, cmd);
    }
  }

  void SceneManager::ApplyPush(std::unique_ptr<Scene> scene,
                                IEngineContext* context)
  {
    // Pause current scene
    if (!m_Stack.empty())
      m_Stack.back()->OnExit();

    scene->SetContext(context);
    m_Stack.push_back(std::move(scene));
    m_Stack.back()->OnEnter();
  }

  void SceneManager::ApplyPop(IEngineContext* context)
  {
    if (m_Stack.empty()) return;

    m_Stack.back()->OnDestroy();
    m_Stack.pop_back();

    // Resume previous scene
    if (!m_Stack.empty())
      m_Stack.back()->OnEnter();
  }

  void SceneManager::ApplyReplace(std::unique_ptr<Scene> scene,
                                   IEngineContext* context)
  {
    if (!m_Stack.empty())
    {
      m_Stack.back()->OnDestroy();
      m_Stack.pop_back();
    }

    scene->SetContext(context);
    m_Stack.push_back(std::move(scene));
    m_Stack.back()->OnEnter();
  }

} // namespace Cadmium
