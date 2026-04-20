#ifndef CADMIUM_SCENE_MANAGER_HPP
#define CADMIUM_SCENE_MANAGER_HPP

#include <cadmium/core/scene.hpp>
#include <memory>
#include <vector>
#include <functional>
#include <variant>

namespace Cadmium
{
  class SceneManager
  {
  public:
    // Active scene accessors
    Scene* GetActiveScene();
    bool   HasActiveScene() const { return !m_Stack.empty(); }

    // Deferred requests
    void RequestPush(std::unique_ptr<Scene> scene);
    void RequestPop();
    void RequestReplace(std::unique_ptr<Scene> scene);

    // Called by engine at end of frame
    void FlushPending(IEngineContext* context);

  private:
    void ApplyPush(std::unique_ptr<Scene> scene, IEngineContext* context);
    void ApplyPop(IEngineContext* context);
    void ApplyReplace(std::unique_ptr<Scene> scene, IEngineContext* context);

    struct PushCmd    { std::unique_ptr<Scene> scene; };
    struct PopCmd     {};
    struct ReplaceCmd { std::unique_ptr<Scene> scene; };

    using Command = std::variant<PushCmd, PopCmd, ReplaceCmd>;

    std::vector<std::unique_ptr<Scene>> m_Stack;
    std::vector<Command> m_Pending;
  };

} // namespace Cadmium

#endif // CADMIUM_SCENE_MANAGER_HPP
